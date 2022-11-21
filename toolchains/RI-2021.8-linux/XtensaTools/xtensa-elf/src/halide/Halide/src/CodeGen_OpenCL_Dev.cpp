#include <algorithm>
#include <sstream>

#include "CodeGen_Internal.h"
#include "CodeGen_OpenCL_Dev.h"
#include "Debug.h"
#include "EliminateBoolVectors.h"
#include "IRMutator.h"
#include "IROperator.h"

namespace Halide {
namespace Internal {

using std::ostringstream;
using std::sort;
using std::string;
using std::vector;

CodeGen_OpenCL_Dev::CodeGen_OpenCL_Dev(Target t) :
    clc(src_stream, t) {
}

string CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::print_type(Type type, AppendSpaceIfNeeded space) {
    ostringstream oss;
    if (type.is_float()) {
        if (type.bits() == 16) {
            oss << "half";
        } else if (type.bits() == 32) {
            oss << "float";
        } else if (type.bits() == 64) {
            oss << "double";
        } else {
            user_error << "Can't represent a float with this many bits in OpenCL C: " << type << "\n";
        }

    } else {
        if (type.is_uint() && type.bits() > 1) oss << 'u';
        switch (type.bits()) {
        case 1:
            internal_assert(type.lanes() == 1) << "Encountered vector of bool\n";
            oss << "bool";
            break;
        case 8:
          oss << "char";
            break;
        case 16:
            oss << "short";
            break;
        case 32:
            oss << "int";
            break;
        case 64:
            oss << "long";
            break;
        default:
            user_error << "Can't represent an integer with this many bits in OpenCL C: " << type << "\n";
        }
    }
    if (type.lanes() != 1) {
        switch (type.lanes()) {
        case 2:
        case 3:
        case 4:
        case 8:
        case 16:
            oss << type.lanes();
            break;
        case 32:
        case 64:
            if (get_target().has_feature(Target::CLCadence)) {
                oss << type.lanes();
                break;
            }
        default:
            user_error <<  "Unsupported vector width in OpenCL C: " << type << "\n";
        }
    }
    if (space == AppendSpace) {
        oss << ' ';
    }
    return oss.str();
}

// These are built-in types in OpenCL
void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::add_vector_typedefs(const std::set<Type> &vector_types) {
}

string CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::print_reinterpret(Type type, Expr e) {
    ostringstream oss;
    oss << "as_" << print_type(type) << "(" << print_expr(e) << ")";
    return oss.str();
}



namespace {
string simt_intrinsic(const string &name) {
    if (ends_with(name, ".__thread_id_x")) {
        return "get_local_id(0)";
    } else if (ends_with(name, ".__thread_id_y")) {
        return "get_local_id(1)";
    } else if (ends_with(name, ".__thread_id_z")) {
        return "get_local_id(2)";
    } else if (ends_with(name, ".__thread_id_w")) {
        return "get_local_id(3)";
    } else if (ends_with(name, ".__block_id_x")) {
        return "get_group_id(0)";
    } else if (ends_with(name, ".__block_id_y")) {
        return "get_group_id(1)";
    } else if (ends_with(name, ".__block_id_z")) {
        return "get_group_id(2)";
    } else if (ends_with(name, ".__block_id_w")) {
        return "get_group_id(3)";
    }
    internal_error << "simt_intrinsic called on bad variable name: " << name << "\n";
    return "";
}
}  // namespace

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const For *loop) {
    if (is_gpu_var(loop->name)) {
        internal_assert((loop->for_type == ForType::GPUBlock) ||
                        (loop->for_type == ForType::GPUThread))
            << "kernel loop must be either gpu block or gpu thread\n";
        internal_assert(is_zero(loop->min));

        do_indent();
        stream << print_type(Int(32)) << " " << print_name(loop->name)
               << " = " << simt_intrinsic(loop->name) << ";\n";

        loop->body.accept(this);

    } else if (loop->for_type == ForType::DMA) {
      // Do nothing - should be ignored in favor of the intrinsic call, but 
      // ealier passes don't like it if this loop were missing
    } else {
        user_assert(loop->for_type != ForType::Parallel) << "Cannot use parallel loops inside OpenCL kernel\n";
        CodeGen_C::visit(loop);
    }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Ramp *op) {
    string id_base = print_expr(op->base);
    string id_stride = print_expr(op->stride);

    ostringstream rhs;
    rhs << id_base << " + " << id_stride << " * ("
        << print_type(op->type.with_lanes(op->lanes)) << ")(0";
    // Note 0 written above.
    for (int i = 1; i < op->lanes; ++i) {
        rhs << ", " << i;
    }
    rhs << ")";
    print_assignment(op->type.with_lanes(op->lanes), rhs.str());
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Broadcast *op) {
    string id_value = print_expr(op->value);

    print_assignment(op->type.with_lanes(op->lanes), id_value);
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Evaluate *op) {
  dma_copy_flag = false;
  if (is_const(op->value)) return;
  string id = print_expr(op->value);
  if(!dma_copy_flag) {
    do_indent();
    stream << "(void)" << id << ";\n";
  }
}

namespace {
// Mapping of integer vector indices to OpenCL ".s" syntax.
const char *vector_elements = "0123456789ABCDEF";

}  // namespace

string CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::get_memory_space(const string &buf) {
    return "__address_space_" + print_name(buf);
}

class LoadExtractor : public IRVisitor {
 public:
    using IRVisitor::visit;
    LoadExtractor() : load(nullptr) {}
    const Load* load;
    void visit( const Load *op ) override {
      load = op;
    }
};

class DMACallExtractor : public IRVisitor {
  public : 
    using IRVisitor::visit;
    
    std::vector<unsigned int> num_dma_calls;
    int last_call_idx;
    
    bool in_dma_loop;
    
    DMACallExtractor() {
      num_dma_calls.clear();
      last_call_idx = -1;
      in_dma_loop   = false;
    }
    
    void end_current_block() {
      if(last_call_idx == -1 || num_dma_calls[last_call_idx]) {
        num_dma_calls.push_back(0);
        last_call_idx++;
      }
    }
    
    // Ensure the internal statements are not dependent on 
    // the current destination buffers for better performance
    
    void visit(const IfThenElse *op) override {
      if(!in_dma_loop) {
        end_current_block();
      }
      
      op->then_case.accept(this);
      
      if(op->else_case.defined())
        op->else_case.accept(this);
    }
    
    void visit(const For *op) override {
      
      if( (op->for_type == ForType::GPUBlock) ||
          (op->for_type == ForType::GPUThread) || 
          (op->for_type == ForType::DMA)  ) {
          
        // Do Nothing
      } else if(!in_dma_loop){
        end_current_block();
      }
      
      // The nested loops are not marked as DMA loops so mark them manually here
      // FIXME : Fix this in Func.cpp where the loop types are set
      if(op->for_type == ForType::DMA)
        in_dma_loop = true;
      
      op->body.accept(this);
      
      if(in_dma_loop == true && op->for_type == ForType::DMA)
        in_dma_loop = false;
    }
    
    void visit(const Call *op) override {
      if (op->is_intrinsic(Call::dma_copy))
      {
        if(last_call_idx == -1) // || num_dma_calls[last_call_idx] == 6)
        {
          last_call_idx++;
          num_dma_calls.push_back(0);
        }
        num_dma_calls[last_call_idx]++;
      }
    }
};

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Call *op) {
    if (op->is_intrinsic(Call::bool_to_mask)) {
        if (op->args[0].type().is_vector()) {
            // The argument is already a mask of the right width. Just
            // sign-extend to the expected type.
            op->args[0].accept(this);
        } else {
            // The argument is a scalar bool. Casting it to an int
            // produces zero or one. Convert it to -1 of the requested
            // type.
            Expr equiv = -Cast::make(op->type, op->args[0]);
            equiv.accept(this);
        }
    } else if (op->is_intrinsic(Call::cast_mask)) {
        // Sign-extension is fine
        Expr equiv = Cast::make(op->type, op->args[0]);
        equiv.accept(this);
    } else if (op->is_intrinsic(Call::select_mask)) {
        internal_assert(op->args.size() == 3);
        string cond = print_expr(op->args[0]);
        string true_val = print_expr(op->args[1]);
        string false_val = print_expr(op->args[2]);

        // Yes, you read this right. OpenCL's select function is declared
        // 'select(false_case, true_case, condition)'.
        ostringstream rhs;
        rhs << "select(" << false_val << ", " << true_val << ", " << cond << ")";
        print_assignment(op->type, rhs.str());
    } else if (op->is_intrinsic(Call::abs)) {
        if (op->type.is_float()) {
            ostringstream rhs;
            rhs << "abs_f" << op->type.bits() << "(" << print_expr(op->args[0]) << ")";
            print_assignment(op->type, rhs.str());
        } else {
            ostringstream rhs;
            rhs << "abs(" << print_expr(op->args[0]) << ")";
            print_assignment(op->type, rhs.str());
        }
    } else if (op->is_intrinsic(Call::absd)) {
        ostringstream rhs;
        rhs << "abs_diff(" << print_expr(op->args[0]) << ", " << print_expr(op->args[1]) << ")";
        print_assignment(op->type, rhs.str());
    } else if (op->is_intrinsic(Call::dma_copy)) {
        
      dma_copy_flag = true;
        std::vector<string> print_args;
        
        // Arguments 0-1 are the dummy loads used for base index calculation
        for (uint i = 0; i < 2; i++) {

          std::stringstream baseptr;
          LoadExtractor ld;
          op->args[i].accept(&ld);

          baseptr << "((" << get_memory_space(ld.load->name) << " "
                  << print_type(op->type) << "*)"
                  << print_name(ld.load->name) << " + " << print_expr(ld.load->index) << ")";
          print_args.push_back(baseptr.str());
        }

        for (uint i = 2; i < op->args.size(); i++) {
          print_args.push_back(print_expr(op->args[i]));
        }
        
        string wait_id  = "evt" + unique_name('_');
        wait_ids.push_back(wait_id);
        
        do_indent();
        stream << "event_t " << wait_id << " = ";
        
        stream << "xt_async_work_group_2d_copy(" << print_args[0];
        
        for (uint i = 1; i < print_args.size(); i++) {
          
          stream << ", " << print_args[i];
          
          // Fix for DMA non char types.
          // Cannot find how to generate sizeof operator, generating here instead
          // This should happen when the new_args are generated in StorageFlatteing.cpp
          
          if(i == 2 || i == 4 || i == 5)
            stream << " * sizeof(" << print_type(op->type) << ")";
        }
        
        stream << ");\n";
        
        // Synchronize
        if(wait_ids.size() == num_dma_calls[curr_dma_block]) {
        
          for(auto i : wait_ids) {
            do_indent();
            stream << "wait_group_events(1, &" << i << ");\n";
          }
          
          curr_dma_block++;
          wait_ids.clear();
        }

    } else if(op->is_intrinsic(Call::count_leading_zeros)) {
         
        ostringstream rhs;
        rhs << "clz(" << print_expr(op->args[0])<< ")";
        print_assignment(op->type, rhs.str());
       
     } else if(op->is_intrinsic(Call::mul_hi)) {
         
        ostringstream rhs;
        rhs << "mul_hi(" 
            << print_expr(op->args[0]) << ","
            << print_expr(op->args[1])
            << ")";
        
        print_assignment(op->type, rhs.str());
       
     } else {
        CodeGen_C::visit(op);
    }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Div *op) {
    if (get_target().has_feature(Target::CLCadence)) {
        Type ty = op->type;
        int bits = 0;

        // No optimization on scalar types and floats.
        if (ty.is_scalar() || ty.is_float()) {
            CodeGen_C::visit(op);
            return;
        }

        if (is_const_power_of_two_integer(op->b, &bits)) {
            CodeGen_C::visit(op);
            return;
        }

        if (ty.is_vector()) {
            // Let default generator handle zero divisor since we don't know what will happen on the device.
            bool can_replace = !is_zero(op->b);
            can_replace &= (ty.is_int() || ty.is_uint());
            can_replace &= (ty.bits() == 32);
            can_replace &= (ty.lanes() == 16 || ty.lanes() == 32);
            if (can_replace) {
                print_expr(Call::make(op->type, "xt_div", {op->a, op->b}, Call::Extern));
                return;
            }
       }
    }

    // Fall through to the default generator.
    CodeGen_C::visit(op);
}

bool expanding_multiply_valid_uint(Expr a)
{
  bool a_isValid = false;
  if (a.as<Cast>() && (a.as<Cast>()->value.type() == UInt(16,32))) {
    a_isValid = true;
  }
  if (a.as<Broadcast>()) {
    const Broadcast * broad_a = a.as<Broadcast>();
    if (broad_a->value.as<Cast>()) {
      const Cast * upcast_a = broad_a->value.as<Cast>();
      if (upcast_a->type == UInt(32) && upcast_a->value.type() == UInt(16))
        a_isValid = true;
    } else if (broad_a->value.as<UIntImm>()) {
      auto val = broad_a->value.as<UIntImm>()->value;
      a_isValid = UInt(16).can_represent(val);
    }else if (broad_a->value.as<IntImm>()) {
      auto val = broad_a->value.as<IntImm>()->value;
      a_isValid = (val >= 0) && UInt(16).can_represent(val);
    }
  }
  
  return a_isValid;
}

bool expanding_multiply_valid_int(Expr a)
{
  bool a_isValid = false;
  if (a.as<Cast>() && (a.as<Cast>()->value.type() == Int(16,32))) {
    a_isValid = true;
  }
  if (a.as<Broadcast>()) {
    const Broadcast * broad_a = a.as<Broadcast>();
    if (broad_a->value.as<Cast>()) {
      const Cast * upcast_a = broad_a->value.as<Cast>();
      if (upcast_a->type == Int(32) && upcast_a->value.type() == Int(16))
        a_isValid = true;
    } else if (broad_a->value.as<IntImm>()) {
      auto val = broad_a->value.as<IntImm>()->value;
      a_isValid = Int(16).can_represent(val);
    } else if (broad_a->value.as<UIntImm>()) {
      auto val = broad_a->value.as<UIntImm>()->value;
      a_isValid = Int(16).can_represent(val);
    }

  }
  
  return a_isValid;
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Mul *op) {
    if (get_target().has_feature(Target::CLCadence)) {
        Type ty = op->type;

        // No optimization on scalar types and floats.
        if (ty.is_scalar() || ty.is_float()) {
            CodeGen_C::visit(op);
            return;
        }

        if (ty.is_vector()) {
        // We want to find the pattern mul(convert_uint32(ushort32), convert_uint32(ushort32))
        // and replace it with xt_mul32(ushort32, ushort32)

        //  std::cout<<"Expr a "<<op->a<<" Expr b "<<op->b <<"\n" ;
        // An operand can be "pierced" if it is a vector upcast, or a broadcast of a scalar upcast
            bool a_isValid_uint = expanding_multiply_valid_uint(op->a);
            bool b_isValid_uint = expanding_multiply_valid_uint(op->b);
            
            bool a_isValid_int  = expanding_multiply_valid_int(op->a);
            bool b_isValid_int  = expanding_multiply_valid_int(op->b);
	    
            if( a_isValid_uint && b_isValid_uint && op->type == UInt(32,32)) {
              Expr true_a;
              if(op->a.as<Cast>()) {
                true_a = op->a.as<Cast>()->value;
              } else if (op->a.as<Broadcast>()->value.as<Cast>()) {
                true_a = Broadcast::make(op->a.as<Broadcast>()->value.as<Cast>()->value, 32);
              } else {
                true_a = Broadcast::make(Cast::make(UInt(16),(op->a.as<Broadcast>()->value)), 32);
              }
              Expr true_b;
              if(op->b.as<Cast>()) {
                true_b = op->b.as<Cast>()->value;
              } else if (op->b.as<Broadcast>()->value.as<Cast>()) {
                true_b = Broadcast::make(op->b.as<Broadcast>()->value.as<Cast>()->value, 32);
              } else {
                true_b = Broadcast::make(Cast::make(UInt(16),(op->b.as<Broadcast>()->value)), 32);
              }
	      Expr ret;	      
	      ret = Call::make(op->type, "xt_mul32", {true_a, true_b}, Call::Extern);
	      print_expr(ret);
              return;
            } else if( a_isValid_int &&  b_isValid_int && op->type == Int(32,32)) {
              Expr true_a;
              if(op->a.as<Cast>()) {
                true_a = op->a.as<Cast>()->value;
	      } else if (op->a.as<Broadcast>()->value.as<Cast>()) {
                true_a = Broadcast::make(op->a.as<Broadcast>()->value.as<Cast>()->value, 32);
              } else {
                true_a = Broadcast::make(Cast::make(Int(16),(op->a.as<Broadcast>()->value)), 32);
              }

              Expr true_b;
              if(op->b.as<Cast>()) {
                true_b = op->b.as<Cast>()->value;
              } else if (op->b.as<Broadcast>()->value.as<Cast>()) {
                true_b = Broadcast::make(op->b.as<Broadcast>()->value.as<Cast>()->value, 32);
              } else {
                true_b = Broadcast::make(Cast::make(Int(16),(op->b.as<Broadcast>()->value)), 32);
              }
              
	      Expr ret;
	      ret = Cast::make(op->type, Call::make(Int(32,32), "xt_mul32", {true_a, true_b}, Call::Extern));
              print_expr(ret);
              return;              
            } else {
              bool can_replace  = (ty.is_int() || ty.is_uint());
              int bits;
              
              // FIXME : Add mirror for op->a
              if(is_const_power_of_two_integer(op->b, &bits)) {
                print_expr(Call::make(op->type, Call::shift_left, {op->a, Expr(bits)}, Call::Intrinsic));
                return;
              } else if (can_replace && (ty.bits() == 32) && (ty.lanes() == 16 || ty.lanes() == 32)) {
                  print_expr(Call::make(op->type, "xt_mul", {op->a, op->b}, Call::Extern));
                  return;
              }
            }
        }
    }

    // Fall through to the default generator.
    CodeGen_C::visit(op);
}

string CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::print_extern_call(const Call *op) {
    internal_assert(!function_takes_user_context(op->name));
    vector<string> args(op->args.size());
    for (size_t i = 0; i < op->args.size(); i++) {
        args[i] = print_expr(op->args[i]);
    }
    ostringstream rhs;
    rhs << op->name << "(" << with_commas(args) << ")";
    return rhs.str();
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Load *op) {
    user_assert(is_one(op->predicate)) << "Predicated load is not supported inside OpenCL kernel.\n";

    // If we're loading a contiguous ramp into a vector, use vload instead.
    Expr ramp_base = strided_ramp_base(op->index);
    if (ramp_base.defined()) {
        internal_assert(op->type.is_vector());
        string id_ramp_base = print_expr(ramp_base);

        ostringstream rhs;
        rhs << "vload" << op->type.lanes()
            << "(0, (" << get_memory_space(op->name) << " "
            << print_type(op->type.element_of()) << "*)"
            << print_name(op->name) << " + " << id_ramp_base << ")";

        print_assignment(op->type, rhs.str());
        return;
    }

    string id_index;
    Type indexType;
    
    // FIXME : print_expr(op->index) generates dead code based on the non replaced ramp
    // It should be fixed by movig this replacement to an earlier phase
    // Unable to replace ramp here as it is a const
    if (op->index.as<Cast>() != nullptr && op->index.as<Cast>()->value.type().bits() < op->index.type().bits() ) {
      id_index = print_expr(op->index.as<Cast>()->value);
      indexType = op->index.as<Cast>()->value.type();
    } else {
      id_index = print_expr(op->index);
      indexType = op->index.type();
    }

    // Get the rhs just for the cache.
    bool type_cast_needed = !(allocations.contains(op->name) &&
                              allocations.get(op->name).type == op->type);
    ostringstream rhs;
    if (type_cast_needed) {
        rhs << "((" << get_memory_space(op->name) << " "
            << print_type(op->type) << " *)"
            << print_name(op->name)
            << ")";
    } else {
        rhs << print_name(op->name);
    }
    rhs << "[" << id_index << "]";

    std::map<string, string>::iterator cached = cache.find(rhs.str());
    if (cached != cache.end()) {
        id = cached->second;
        return;
    }

    if (indexType.is_vector()) {
        // If index is a vector, gather vector elements.
        internal_assert(op->type.is_vector());
        
        Expr new_index  = op->index;
        Expr new_base   = Expr(0);
        
        int num_lanes   = op->index.type().lanes();
        
        if (op->index.as<Ramp>()) {
          auto temp = op->index.as<Ramp>();

          new_base    = temp->base;
          new_index   = Ramp::make(Expr(0), temp->stride, temp->lanes);
          num_lanes   = temp->lanes;
          
        } else if(op->index.as<Add>()) {
          auto temp = op->index.as<Add>();
          
          // The offset will be broadcast?
          
          if(temp->a.as<Broadcast>()) {
            
            new_base  = temp->a.as<Broadcast>()->value;
            new_index = temp->b;
            num_lanes = temp->a.as<Broadcast>()->lanes;
            
          } else if(temp->b.as<Broadcast>()) {
            
            new_base  = temp->b.as<Broadcast>()->value;
            new_index = temp->a;
            num_lanes = temp->b.as<Broadcast>()->lanes;
            
          }
        }
        
        if(
            op->type == Int(8, num_lanes)   || 
            op->type == UInt(8, num_lanes)  || 
            op->type == Int(16, num_lanes)  || 
            op->type == UInt(16, num_lanes) || 
            op->type == Float(16, num_lanes)
          ) {
          
          new_index   = Cast::make(UInt(16, num_lanes), new_index);
          
        } else if (
                  op->type == Int(32, num_lanes)   || 
                  op->type == UInt(32, num_lanes)  || 
                  op->type == Float(32, num_lanes)
                ) {
                
          new_index   = Cast::make(UInt(32, num_lanes), new_index);
        } else {
          user_assert(0) << "Invalid xt_gather Operation";
        }
        
        auto base     = print_expr(new_base);
        auto index    = print_expr(new_index);
        
        id = "_" + unique_name('V');
        cache[rhs.str()] = id;
        
        if(target.has_feature(Target::CLCadence) &&
             (buffers_memory_type.find(op->name)->second == MemoryType::GPUShared  ||
              buffers_memory_type.find(op->name)->second == MemoryType::Xt_Shared)){
            do_indent();
            stream
                << print_type(op->type) << " " << id << " = "
                << "xt_gather" << op->type.lanes() << "(("
                << get_memory_space(op->name) << " "
                << print_type(op->type.element_of()) << "*)";
            
            stream
                << print_name(op->name) << " + " << base << ", "
                << "(" << print_type(new_index.type().element_of()) << "*)";
            
            if(!new_index.type().is_handle())
                stream << "&";
            stream << index << ");\n";
            
            return;
        }

        do_indent();
        stream << print_type(op->type)
               << " " << id << ";\n";

        for (int i = 0; i < op->type.lanes(); ++i) {
            do_indent();
            stream
                << id << ".s" << vector_elements[i]
                << " = ((" << get_memory_space(op->name) << " "
                << print_type(op->type.element_of()) << "*)"
                << print_name(op->name) << ")"
                << "[" << id_index << ".s" << vector_elements[i] << "];\n";
        }
    } else {
        print_assignment(op->type, rhs.str());
    }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Store *op) {
    user_assert(is_one(op->predicate)) << "Predicated store is not supported inside OpenCL kernel.\n";

    string id_value = print_expr(op->value);
    Type t = op->value.type();

    // If we're writing a contiguous ramp, use vstore instead.
    Expr ramp_base = strided_ramp_base(op->index);
    if (ramp_base.defined()) {
        internal_assert(op->value.type().is_vector());
        string id_ramp_base = print_expr(ramp_base);

        do_indent();
        stream << "vstore" << t.lanes() << "("
               << id_value << ","
               << 0 << ", (" << get_memory_space(op->name) << " "
               << print_type(t.element_of()) << "*)"
               << print_name(op->name) << " + " << id_ramp_base
               << ");\n";

    } else if (op->index.type().is_vector()) {
        // If index is a vector, scatter vector elements.
        internal_assert(t.is_vector());

        string id_index = print_expr(op->index);

        for (int i = 0; i < t.lanes(); ++i) {
            do_indent();
            stream << "((" << get_memory_space(op->name) << " "
                   << print_type(t.element_of()) << " *)"
                   << print_name(op->name)
                   << ")["
                   << id_index << ".s" << vector_elements[i] << "] = "
                   << id_value << ".s" << vector_elements[i] << ";\n";
        }
    } else {
        bool type_cast_needed = !(allocations.contains(op->name) &&
                                  allocations.get(op->name).type == t);

        string id_index = print_expr(op->index);
        string id_value = print_expr(op->value);
        do_indent();

        if (type_cast_needed) {
            stream << "(("
                   << get_memory_space(op->name) << " "
                   << print_type(t)
                   << " *)"
                   << print_name(op->name)
                   << ")";
        } else {
            stream << print_name(op->name);
        }
        stream << "[" << id_index << "] = "
               << id_value << ";\n";
    }

    cache.clear();
}

namespace {
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const EQ *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, "==");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const NE *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, "!=");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const LT *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, "<");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const LE *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, "<=");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const GT *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, ">");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const GE *op) {
    visit_binop(eliminated_bool_type(op->type, op->a.type()), op->a, op->b, ">=");
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Cast *op) {

  if (op->type.is_vector()) {
    if(op->type == UInt(16,32)) {
      if(op->value.as<Add>()) {
	/**
	   FIXME: Currently we are handling only the 
	   following pattern: 
	   A*B +C.
	   This is just an optimization to improve the generated
	   code's execution speed.
	**/
	Expr ab = op->value.as<Add>()->a;
	Expr c = op->value.as<Add>()->b;
	if(ab.as<Mul>()) {
	  auto a = ab.as<Mul>()->a;
	  auto b = ab.as<Mul>()->b;
	  if(a.as<Cast>() && b.as<Broadcast>() && c.as<Cast>()) {
	    auto a_orig = a.as<Cast>()->value;
	    auto c_orig = c.as<Cast>()->value;
	    auto b_val  = (b.as<Broadcast>()->value).as<IntImm>()->value;
	    auto b_val_short = UIntImm::make(UInt(16), b_val);
	    //FIXME : Check if b_val is in range
	    auto a_print = print_expr(a_orig);
	    auto b_print = print_expr(Broadcast::make(b_val_short, 32));
	    auto c_print = print_expr(c_orig);
	    print_assignment(op->type, "(" + a_print  + " * " + b_print + ") + " + c_print);
	    return;
	  }
	}
      }
    }
    print_assignment(op->type, "convert_" + print_type(op->type) + "(" + print_expr(op->value) + ")");
  } else {
    CodeGen_C::visit(op);
  }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Select *op) {
    internal_assert(op->condition.type().is_scalar());
    CodeGen_C::visit(op);
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Allocate *op) {
    user_assert(!op->new_expr.defined()) << "Allocate node inside OpenCL kernel has custom new expression.\n" <<
        "(Memoization is not supported inside GPU kernels at present.)\n";

    buffers_memory_type.insert(std::pair<string , MemoryType>(op->name, op->memory_type));
    if (op->name == "__shared") {
        // Already handled
        op->body.accept(this);
    } else {
        open_scope();

        debug(2) << "Allocate " << op->name << " on device\n";

        debug(3) << "Pushing allocation called " << op->name << " onto the symbol table\n";

        // Allocation is not a shared memory allocation, just make a local declaration.
        // It must have a constant size.
        int32_t size = op->constant_allocation_size();
        user_assert(size > 0)
            << "Allocation " << op->name << " has a dynamic size. "
            << "Only fixed-size allocations are supported on the gpu. "
            << "Try storing into shared memory instead.";

        do_indent();
        if(op->memory_type ==  MemoryType::Xt_Shared && target.has_feature(Target::CLCadence)){
            // Do Nothing. Handled already
        }else{
            stream << print_type(op->type) << ' '
                << print_name(op->name) << "[" << size << "];\n";
            do_indent();
            stream << "#define " << get_memory_space(op->name) << " __private\n";
        }
        Allocation alloc;
        alloc.type = op->type;
        allocations.push(op->name, alloc);

        op->body.accept(this);

        // Should have been freed internally
        internal_assert(!allocations.contains(op->name));

        close_scope("alloc " + print_name(op->name));
    }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Free *op) {
    if (op->name == "__shared") {
        return;
    } else {
        // Should have been freed internally
        internal_assert(allocations.contains(op->name));
        allocations.pop(op->name);
        do_indent();
        stream << "#undef " << get_memory_space(op->name) << "\n";
    }
}


void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const AssertStmt *op) {
    user_warning << "Ignoring assertion inside OpenCL kernel: " << op->condition << "\n";
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Shuffle *op) {
    if (op->is_interleave()) {
        int op_lanes = op->type.lanes();
        internal_assert(!op->vectors.empty());
        int arg_lanes = op->vectors[0].type().lanes();
        if (op->vectors.size() == 1) {
            // 1 argument, just do a simple assignment
            internal_assert(op_lanes == arg_lanes);
            print_assignment(op->type, print_expr(op->vectors[0]));
        } else if (op->vectors.size() == 2) {
            // 2 arguments, set the .even to the first arg and the
            // .odd to the second arg
            internal_assert(op->vectors[1].type().lanes() == arg_lanes);
            internal_assert(op_lanes / 2 == arg_lanes);
            string a1 = print_expr(op->vectors[0]);
            string a2 = print_expr(op->vectors[1]);
            id = unique_name('_');
            do_indent();
            stream << print_type(op->type) << " " << id << ";\n";
            do_indent();
            stream << id << ".even = " << a1 << ";\n";
            do_indent();
            stream << id << ".odd = " << a2 << ";\n";
        } else {
            // 3+ arguments, interleave via a vector literal
            // selecting the appropriate elements of the vectors
            int dest_lanes = op->type.lanes();
            int max_lanes = get_target().has_feature(Target::CLCadence) ? 64 : 16;
            internal_assert(dest_lanes <= max_lanes);
            int num_vectors = op->vectors.size();
            vector<string> arg_exprs(num_vectors);
            for (int i = 0; i < num_vectors; i++) {
                internal_assert(op->vectors[i].type().lanes() == arg_lanes);
                arg_exprs[i] = print_expr(op->vectors[i]);
            }
            internal_assert(num_vectors * arg_lanes >= dest_lanes);
            id = unique_name('_');
            do_indent();
            stream << print_type(op->type) << " " << id;
            stream << " = (" << print_type(op->type) << ")(";
            for (int i = 0; i < dest_lanes; i++) {
                int arg = i % num_vectors;
                int arg_idx = i / num_vectors;
                internal_assert(arg_idx <= arg_lanes);
                stream << arg_exprs[arg] << ".s" << vector_elements[arg_idx];
                if (i != dest_lanes - 1) {
                    stream << ", ";
                }
            }
            stream << ");\n";
        }
    } else {
        std::string maskshift = unique_name('_');
        do_indent();
        auto mask_type = op->type;
        if(op->type.is_int() || op->type.is_float())
        {
          mask_type = UInt(op->type.bits(), op->type.lanes());
        }
        stream << print_type(mask_type) << " " << maskshift;
        stream << " = {";
        int dest_lanes = op->type.lanes();
        for(int i = 0; i < dest_lanes; i++) {
          stream << op->indices[i];
          if(i != dest_lanes - 1)
            stream <<  ", ";
        }
        stream << "};\n";

        ostringstream rhs;
        rhs << "shuffle2("
            << print_expr(op->vectors[0]) << ", "
            << print_expr(op->vectors[1]) << ", "
            << maskshift << ")";

        print_assignment(op->type, rhs.str());
    }
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Max *op) {
    print_expr(Call::make(op->type, "max", {op->a, op->b}, Call::Extern));
}

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::visit(const Min *op) {
    print_expr(Call::make(op->type, "min", {op->a, op->b}, Call::Extern));
}

void CodeGen_OpenCL_Dev::add_kernel(Stmt s,
                                    const string &name,
                                    const vector<DeviceArgument> &args) {
    debug(2) << "CodeGen_OpenCL_Dev::compile " << name << "\n";

    // TODO: do we have to uniquify these names, or can we trust that they are safe?
    cur_kernel_name = name;
    clc.add_kernel(s, name, args);
}

namespace {
struct BufferSize {
    string name;
    size_t size;

    BufferSize() : size(0) {}
    BufferSize(string name, size_t size) : name(name), size(size) {}

    bool operator < (const BufferSize &r) const {
        return size < r.size;
    }
};
}  // namespace

struct BufferDetails {
  string name;
  Type type;
  int32_t size;
  
  BufferDetails(string n, Type t, int32_t s) {
    name = n;
    type = t;
    size = s;
  }
};

class LocalBufferExtractor : public IRVisitor {
  public : 
    using IRVisitor::visit;
    
    std::vector<BufferDetails> local_buffers;
    bool has_feature_CLCadence;
    
    LocalBufferExtractor(bool has_feature = false) {
      local_buffers.clear();
      has_feature_CLCadence = has_feature;
    }
    
    void visit(const Allocate *op) {
      if (  has_feature_CLCadence && 
            op->name != "__shared" &&  op->memory_type == MemoryType::Xt_Shared) {
        local_buffers.push_back(BufferDetails(op->name, op->type, op->constant_allocation_size()));
      }
      op->body.accept(this);
    }
};

void CodeGen_OpenCL_Dev::CodeGen_OpenCL_C::add_kernel(Stmt s,
                                                      const string &name,
                                                      const vector<DeviceArgument> &args) {

    debug(2) << "Adding OpenCL kernel " << name << "\n";

    debug(2) << "Eliminating bool vectors\n";
    s = eliminate_bool_vectors(s);
    debug(2) << "After eliminating bool vectors:\n" << s << "\n";
    
    DMACallExtractor d;
    s.accept(&d);
    
    this->num_dma_calls   = d.num_dma_calls;
    this->curr_dma_block  = 0;
    this->wait_ids.clear();
    
    LocalBufferExtractor l(target.has_feature(Target::CLCadence));
    s.accept(&l);
    
    // Figure out which arguments should be passed in __constant.
    // Such arguments should be:
    // - not written to,
    // - loads are block-uniform,
    // - constant size,
    // - and all allocations together should be less than the max constant
    //   buffer size given by CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE.
    // The last condition is handled via the preprocessor in the kernel
    // declaration.
    vector<BufferSize> constants;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer &&
            CodeGen_GPU_Dev::is_buffer_constant(s, args[i].name) &&
            args[i].size > 0) {
            constants.push_back(BufferSize(args[i].name, args[i].size));
        }
    }

    // Sort the constant candidates from smallest to largest. This will put
    // as many of the constant allocations in __constant as possible.
    // Ideally, we would prioritize constant buffers by how frequently they
    // are accessed.
    sort(constants.begin(), constants.end());

    // Compute the cumulative sum of the constants.
    for (size_t i = 1; i < constants.size(); i++) {
        constants[i].size += constants[i - 1].size;
    }

    // Create preprocessor replacements for the address spaces of all our buffers.
    stream << "// Address spaces for " << name << "\n";
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            vector<BufferSize>::iterator constant = constants.begin();
            while (constant != constants.end() &&
                   constant->name != args[i].name) {
                constant++;
            }

            if (constant != constants.end()) {
                stream << "#if " << constant->size << " <= MAX_CONSTANT_BUFFER_SIZE && "
                       << constant - constants.begin() << " < MAX_CONSTANT_ARGS\n";
                stream << "#define " << get_memory_space(args[i].name) << " __constant\n";
                stream << "#else\n";
                stream << "#define " << get_memory_space(args[i].name) << " __global\n";
                stream << "#endif\n";
            } else {
                stream << "#define " << get_memory_space(args[i].name) << " __global\n";
            }
        }
    }

    // Emit the function prototype
    stream << "__kernel void " << name << "(\n";
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            stream << " " << get_memory_space(args[i].name) << " ";
            if (!args[i].write) stream << "const ";
            stream << print_type(args[i].type) << " *"
                   << "restrict "
                   << print_name(args[i].name);
            Allocation alloc;
            alloc.type = args[i].type;
            allocations.push(args[i].name, alloc);
        } else {
            Type t = args[i].type;
            // Bools are passed as a uint8.
            t = t.with_bits(t.bytes() * 8);
            stream << " const "
                   << print_type(t)
                   << " "
                   << print_name(args[i].name);
        }

        if (i < args.size()-1) stream << ",\n";
    }
    stream << ",\n" << " __address_space___shared int16* __shared";

    stream << ")\n";

    open_scope();
    
    for(auto i : l.local_buffers) {
      do_indent();
      stream << "__local" << ' '
                << print_type(i.type) << ' '
                << print_name(i.name) << "[" << i.size << "];\n";
      
      do_indent();
      stream << "#define " << get_memory_space(i.name) << " __local\n";  
    }
    
    print(s);
    
    for(auto i : l.local_buffers) {
      do_indent();
      stream << "#undef " << get_memory_space(i.name) << "\n";
    }
    
    close_scope("kernel " + name);

    for (size_t i = 0; i < args.size(); i++) {
        // Remove buffer arguments from allocation scope
        if (args[i].is_buffer) {
            allocations.pop(args[i].name);
        }
    }

    // Undef all the buffer address spaces, in case they're different in another kernel.
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i].is_buffer) {
            stream << "#undef " << get_memory_space(args[i].name) << "\n";
        }
    }
}

void CodeGen_OpenCL_Dev::init_module() {
    debug(2) << "OpenCL device codegen init_module\n";

    // wipe the internal kernel source
    src_stream.str("");
    src_stream.clear();

    const Target &target = clc.get_target();

    // This identifies the program as OpenCL C (as opposed to SPIR).
    src_stream << "/*OpenCL C " << target.to_string() << "*/\n";

    src_stream << "#pragma OPENCL FP_CONTRACT ON\n";

    // Write out the Halide math functions.
    src_stream << "inline float float_from_bits(unsigned int x) {return as_float(x);}\n"
               << "inline float nan_f32() { return NAN; }\n"
               << "inline float neg_inf_f32() { return -INFINITY; }\n"
               << "inline float inf_f32() { return INFINITY; }\n"
               << "#define sqrt_f32 sqrt \n"
               << "#define sin_f32 sin \n"
               << "#define cos_f32 cos \n"
               << "#define exp_f32 exp \n"
               << "#define log_f32 log \n"
               << "#define abs_f32 fabs \n"
               << "#define floor_f32 floor \n"
               << "#define ceil_f32 ceil \n"
               << "#define round_f32 round \n"
               << "#define trunc_f32 trunc \n"
               << "#define pow_f32 pow\n"
               << "#define asin_f32 asin \n"
               << "#define acos_f32 acos \n"
               << "#define tan_f32 tan \n"
               << "#define atan_f32 atan \n"
               << "#define atan2_f32 atan2\n"
               << "#define sinh_f32 sinh \n"
               << "#define asinh_f32 asinh \n"
               << "#define cosh_f32 cosh \n"
               << "#define acosh_f32 acosh \n"
               << "#define tanh_f32 tanh \n"
               << "#define atanh_f32 atanh \n"
               << "#define fast_inverse_f32 native_recip \n"
               << "#define fast_inverse_sqrt_f32 native_rsqrt \n"
               << "inline int halide_gpu_thread_barrier() {\n"
               << "  barrier(CLK_LOCAL_MEM_FENCE);\n" // Halide only ever needs local memory fences.
               << "  return 0;\n"
               << "}\n";

    if (target.has_feature(Target::CLCadence))
        src_stream <<"#define select(a,b,c) (c ? b : a)\n"
                   <<"#define halide_gpu_thread_barrier() 0; barrier(CLK_LOCAL_MEM_FENCE)\n";
               
    // __shared always has address space __local.
    src_stream << "#define __address_space___shared __local\n";

    if (target.has_feature(Target::CLDoubles)) {
        src_stream << "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n"
                   << "bool is_nan_f64(double x) {return x != x; }\n"
                   << "#define sqrt_f64 sqrt\n"
                   << "#define sin_f64 sin\n"
                   << "#define cos_f64 cos\n"
                   << "#define exp_f64 exp\n"
                   << "#define log_f64 log\n"
                   << "#define abs_f64 fabs\n"
                   << "#define floor_f64 floor\n"
                   << "#define ceil_f64 ceil\n"
                   << "#define round_f64 round\n"
                   << "#define trunc_f64 trunc\n"
                   << "#define pow_f64 pow\n"
                   << "#define asin_f64 asin\n"
                   << "#define acos_f64 acos\n"
                   << "#define tan_f64 tan\n"
                   << "#define atan_f64 atan\n"
                   << "#define atan2_f64 atan2\n"
                   << "#define sinh_f64 sinh\n"
                   << "#define asinh_f64 asinh\n"
                   << "#define cosh_f64 cosh\n"
                   << "#define acosh_f64 acosh\n"
                   << "#define tanh_f64 tanh\n"
                   << "#define atanh_f64 atanh\n";
    }

    if (target.has_feature(Target::CLHalf)) {
        src_stream << "#pragma OPENCL EXTENSION cl_khr_fp16 : enable\n"
                   << "inline half half_from_bits(unsigned short x) {return as_half(x);}\n"
                   << "inline half nan_f16() { return nan((unsigned short)0); }\n"
                   << "inline half neg_inf_f16() { return -INFINITY; }\n"
                   << "inline half inf_f16() { return INFINITY; }\n"
                   << "bool is_nan_f16(half x) {return x != x; }\n"
                   << "#define sqrt_f16 sqrt\n"
                   << "#define sin_f16 sin\n"
                   << "#define cos_f16 cos\n"
                   << "#define exp_f16 exp\n"
                   << "#define log_f16 log\n"
                   << "#define abs_f16 fabs\n"
                   << "#define floor_f16 floor\n"
                   << "#define ceil_f16 ceil\n"
                   << "#define round_f16 round\n"
                   << "#define trunc_f16 trunc\n"
                   << "#define pow_f16 pow\n"
                   << "#define asin_f16 asin\n"
                   << "#define acos_f16 acos\n"
                   << "#define tan_f16 tan\n"
                   << "#define atan_f16 atan\n"
                   << "#define atan2_f16 atan2\n"
                   << "#define sinh_f16 sinh\n"
                   << "#define asinh_f16 asinh\n"
                   << "#define cosh_f16 cosh\n"
                   << "#define acosh_f16 acosh\n"
                   << "#define tanh_f16 tanh\n"
                   << "#define atanh_f16 atanh\n";
    }

    src_stream << '\n';

    // Add at least one kernel to avoid errors on some implementations for functions
    // without any GPU schedules.
    src_stream << "__kernel void _at_least_one_kernel(int x) { }\n";

    cur_kernel_name = "";
}

vector<char> CodeGen_OpenCL_Dev::compile_to_src() {
    string str = src_stream.str();
    debug(1) << "OpenCL kernel:\n" << str << "\n";
    vector<char> buffer(str.begin(), str.end());
    buffer.push_back(0);
    return buffer;
}

string CodeGen_OpenCL_Dev::get_current_kernel_name() {
    return cur_kernel_name;
}

void CodeGen_OpenCL_Dev::dump() {
    std::cerr << src_stream.str() << std::endl;
}

std::string CodeGen_OpenCL_Dev::print_gpu_name(const std::string &name) {
    return name;
}

}  // namespace Internal
}  // namespace Halide
