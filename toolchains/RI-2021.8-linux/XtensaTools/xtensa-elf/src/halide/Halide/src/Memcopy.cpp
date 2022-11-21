#include <algorithm>
#include <map>
#include <string>
#include <numeric>

#include "Memcopy.h"
#include "Bounds.h"
#include "ExprUsesVar.h"
#include "IRMutator.h"
#include "Scope.h"
#include "Simplify.h"
#include "IREquality.h"
#include "Util.h"

namespace Halide {
namespace Internal {

using std::map;
using std::set;
using std::string;
using std::vector;

namespace {

class InjectMemcopy : public IRMutator2 {
public:
    InjectMemcopy(const map<string, Function> &e) : env(e) {}

private:
    const map<string, Function> &env;
    std::map<string, const Call*> from_buffer_map;
    const Provide* to_buffer;

private:
    using IRMutator2::visit;

    Expr visit(const Call *op) override {
      if (op->call_type == Call::Image ||
          op->call_type == Call::Halide) {
        from_buffer_map[op->name] = op;
      }
      return op;
    }

    Stmt visit(const Provide *op) override {
      to_buffer = op;
      mutate(op->values[0]);
      return op;
    }

    Stmt visit(const For *loop) override {
        Stmt unmodified = IRMutator2::visit(loop);
        if (loop->for_type == ForType::DMA) {
            
            from_buffer_map.clear();
            to_buffer = NULL;
            Stmt body = mutate(loop->body);
            
            // FIXME : Should be based on the checks from bounds below
            // Ideally the boxes of source and destination should match
            // Here they dont.
            // 
            // Try comparing the bounds of the provider nodes which are 
            // non zero to the required nodes bounds. And pick the node
            // with non-zero bounds?
            
            // Find input/output buffer variables and index ranges associated with them
            std::map<string, Box> boxes       = boxes_touched(loop);
            std::map<string, Box> boxes_src   = boxes_required(loop);
            std::map<string, Box> boxes_dest  = boxes_provided(loop);

            std::vector<string> valid_boxes_src_name;
            
            user_assert(boxes_dest.size() == 1)
              << "Invalid DMA copy loop. Found multiple destination buffers\n";
            
            auto dst_box  = boxes_dest[to_buffer->name];
            std::vector <Expr> dst_box_bounds;
            
            for(unsigned int i = 0; i < dst_box.size(); i++) {
              auto min  = simplify(dst_box[i].min);
              auto max  = simplify(dst_box[i].max);
              Expr diff = simplify(mutate(max) - mutate(min) + 1);
              dst_box_bounds.push_back(diff);
            }
            
            size_t min_dim = dst_box.size();
            
            for (auto entry : boxes_src)
              min_dim   = std::min(min_dim, entry.second.size());
            
            for (auto entry : boxes_src) {
              bool valid_src_flag = true;
              
              for(unsigned int i = 0; i < entry.second.size(); i++) {
                auto min  = simplify(entry.second[i].min);
                auto max  = simplify(entry.second[i].max);
                Expr diff = simplify(mutate(max) - mutate(min) + 1);
                
                if(i < min_dim)
                  if(diff.as<UIntImm>() || diff.as<IntImm>())
                    if(!equal(diff, dst_box_bounds[i]))
                      if(valid_src_flag)
                        valid_src_flag = false;
              }
              
              if(valid_src_flag)
                valid_boxes_src_name.push_back(entry.first);
            }
            
            
            user_assert(valid_boxes_src_name.size() > 0)
              << "Invalid DMA copy loop. No valid dma source found\n";
            
            const Call *from_buffer = from_buffer_map[valid_boxes_src_name[0]];
            
            user_assert(from_buffer != NULL)
              << "Invalid DMA copy loop.  Cannot find DMA source buffer\n";
            user_assert(to_buffer != NULL)
              << "Invalid DMA copy loop.  Cannot find DMA destination buffer\n";
            
            user_assert(boxes.find(from_buffer->name) != boxes.end())
              << "Invalid DMA copy loop.\n"
              << "  Accesses buffer " << from_buffer->name << " but cannot infer access bounds.\n";
            user_assert(boxes.find(to_buffer->name) != boxes.end())
              << "Invalid DMA copy loop.\n"
              << "  Accesses buffer " << to_buffer->name << " but cannot infer access bounds.\n";

            std::vector<Expr> args;

            args.push_back(Variable::make(Handle(), env.at(to_buffer->name).name()));
            
            if (from_buffer->param.defined()) {
              Expr e = Variable::make(Handle(), from_buffer->name, from_buffer->param);
              args.push_back(e);
            } else if (from_buffer->image.defined())
              args.push_back(Variable::make(Handle(), from_buffer->name, from_buffer->image));
            else
              args.push_back(Variable::make(Handle(), from_buffer->name));
            
            unsigned int size = std::min(boxes.at(from_buffer->name).size(), boxes.at(to_buffer->name).size());
            for(unsigned int i = 0; i < size; i++) {
              args.push_back(boxes.at(from_buffer->name)[i].min);
              args.push_back(boxes.at(from_buffer->name)[i].max);
            }
            
            for(unsigned int i = 0; i < size; i++) {
              args.push_back(boxes.at(to_buffer->name)[i].min);
            }
            
            Stmt inject = Evaluate::make(Call::make(from_buffer->type, "dma_copy", args, Call::Intrinsic));

            return Block::make(unmodified, inject);

        } else {
            return unmodified;
        }
    }
};

} // anonymous namespace

Stmt inject_memcopy(Stmt s, const map<string, Function> &env) {
    
    Stmt stmt = InjectMemcopy(env).mutate(s);
    
    return stmt;
}

}  // namespace Internal
}  // namespace Halide
