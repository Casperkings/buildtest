/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 *  Main authors:
 *     Christian Schulte <schulte@gecode.org>
 *
 *  Copyright:
 *     Christian Schulte, 2009
 *
 *  This file is part of Gecode, the generic constraint
 *  development environment:
 *     http://www.gecode.org
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

namespace Gecode { namespace Search { namespace Par {

  /*
   * Engine: basic access routines
   */
  template<class Tracer>
  forceinline BAB<Tracer>&
  BAB<Tracer>::Worker::engine(void) const {
    return static_cast<BAB&>(_engine);
  }
  template<class Tracer>
  forceinline typename BAB<Tracer>::Worker*
  BAB<Tracer>::worker(unsigned int i) const {
    return _worker[i];
  }

  template<class Tracer>
  forceinline void
  BAB<Tracer>::Worker::reset(Space* s, unsigned int ngdl) {
    tracer.round();
    delete cur;
    delete best;
    best = NULL;
    path.reset((s == NULL) ? 0 : ngdl);
    d = 0;
    mark = 0;
    idle = false;
    if ((s == NULL) || (s->status(*this) == SS_FAILED)) {
      delete s;
      cur = NULL;
    } else {
      cur = s;
    }
    Search::Worker::reset();
  }


  /*
   * Engine: initialization
   */
  template<class Tracer>
  forceinline
  BAB<Tracer>::Worker::Worker(Space* s, BAB& e)
    : Engine<Tracer>::Worker(s,e), mark(0), best(NULL) {}

  template<class Tracer>
  forceinline
  BAB<Tracer>::BAB(Space* s, const Options& o)
    : Engine<Tracer>(o), best(NULL) {
    WrapTraceRecorder::engine(o.tracer, SearchTracer::EngineType::DFS,
                              workers());
    // Create workers
    _worker = static_cast<Worker**>
      (heap.ralloc(workers() * sizeof(Worker*)));
    // The first worker gets the entire search tree
    _worker[0] = new Worker(s,*this);
    // All other workers start with no work
    for (unsigned int i=1U; i<workers(); i++)
      _worker[i] = new Worker(NULL,*this);
    // Block all workers
    block();
    // Create and start threads
    for (unsigned int i=0U; i<workers(); i++)
      Support::Thread::run(_worker[i]);
  }


  /*
   * Engine: search control
   */
  template<class Tracer>
  forceinline void
  BAB<Tracer>::Worker::better(Space* b) {
    m.acquire();
    delete best;
    best = b->clone();
    mark = path.entries();
    if (cur != NULL)
      cur->constrain(*best);
    m.release();
  }
  template<class Tracer>
  forceinline void
  BAB<Tracer>::solution(Space* s) {
    m_search.acquire();
    if (best != NULL) {
      s->constrain(*best);
      if (s->status() == SS_FAILED) {
        delete s;
        m_search.release();
        return;
      } else {
        delete best;
        best = s->clone();
      }
    } else {
      best = s->clone();
    }
    // Announce better solutions
    for (unsigned int i=0U; i<workers(); i++)
      worker(i)->better(best);
    bool bs = signal();
    solutions.push(s);
    if (bs)
      e_search.signal();
    m_search.release();
  }


  /*
   * Worker: finding and stealing working
   */
  template<class Tracer>
  forceinline void
  BAB<Tracer>::Worker::find(void) {
    // Try to find new work (even if there is none)
    for (unsigned int i=0U; i<engine().workers(); i++) {
      unsigned long int r_d = 0ul;
      typename Engine<Tracer>::Worker* wi = engine().worker(i);
      if (Space* s = wi->steal(r_d,wi->tracer,tracer)) {
        // Reset this guy
        m.acquire();
        idle = false;
        // Not idle but also does not have the root of the tree
        path.ngdl(0);
        d = 0;
        cur = s;
        mark = 0;
        if (best != NULL)
          cur->constrain(*best);
        Statistics t = *this;
        Search::Worker::reset(r_d);
        (*this) += t;
        m.release();
        return;
      }
    }
  }

  /*
   * Statistics
   */
  template<class Tracer>
  Statistics
  BAB<Tracer>::statistics(void) const {
    Statistics s;
    for (unsigned int i=0U; i<workers(); i++)
      s += worker(i)->statistics();
    return s;
  }

  template<class Tracer>
  void
  BAB<Tracer>::constrain(const Space& b) {
    m_search.acquire();
    if (best != NULL) {
      best->constrain(b);
      if (best->status() != SS_FAILED) {
        m_search.release();
        return;
      }
      delete best;
    }
    best = b.clone();
    // Announce better solutions
    for (unsigned int i=0U; i<workers(); i++)
      worker(i)->better(best);
    m_search.release();
  }

  /*
   * Actual work
   */
  template<class Tracer>
  void
  BAB<Tracer>::Worker::run(void) {
    /*
     * The engine maintains the following invariant:
     *  - If the current space (cur) is not NULL, the path always points
     *    to exactly that space.
     *  - If the current space (cur) is NULL, the path always points
     *    to the next space (if there is any).
     *
     * This invariant is needed so that no-goods can be extracted properly
     * when the engine is stopped or has found a solution.
     *
     * An additional invariant maintained by the engine is:
     *   For all nodes stored at a depth less than mark, there
     *   is no guarantee of betterness. For those above the mark,
     *   betterness is guaranteed.
     *
     */
    // Peform initial delay, if not first worker
    if (this != engine().worker(0))
      Support::Thread::sleep(Config::initial_delay);
    // Okay, we are in business, start working
    while (true) {
      switch (engine().cmd()) {
      case C_WAIT:
        // Wait
        engine().wait();
        break;
      case C_TERMINATE:
        // Acknowledge termination request
        engine().ack_terminate();
        // Wait until termination can proceed
        engine().wait_terminate();
        // Thread will be terminated by returning from run
        return;
      case C_RESET:
        // Acknowledge reset request
        engine().ack_reset_start();
        // Wait until reset has been performed
        engine().wait_reset();
        // Acknowledge that reset cycle is over
        engine().ack_reset_stop();
        break;
      case C_WORK:
        // Perform exploration work
        {
          m.acquire();
          if (idle) {
            m.release();
            // Try to find new work
            find();
          } else if (cur != NULL) {
            start();
            if (stop(engine().opt())) {
              // Report stop
              m.release();
              engine().stop();
            } else {
              node++;
              SearchTracer::EdgeInfo ei;
              if (tracer) {
                if (path.entries() > 0) {
                  typename Path<Tracer>::Edge& top = path.top();
                  ei.init(tracer.wid(), top.nid(), top.truealt(),
                          *cur, *top.choice());
                } else if (*tracer.ei()) {
                  ei = *tracer.ei();
                  tracer.invalidate();
                }
              }
              unsigned int nid = tracer.nid();
              switch (cur->status(*this)) {
              case SS_FAILED:
                if (tracer) {
                  SearchTracer::NodeInfo ni(SearchTracer::NodeType::FAILED,
                                            tracer.wid(), nid, *cur);
                  tracer.node(ei,ni);
                }
                fail++;
                delete cur;
                cur = NULL;
                path.next();
                m.release();
                break;
              case SS_SOLVED:
                {
                  if (tracer) {
                    SearchTracer::NodeInfo ni(SearchTracer::NodeType::SOLVED,
                                              tracer.wid(), nid, *cur);
                    tracer.node(ei,ni);
                  }
                  // Deletes all pending branchers
                  (void) cur->choice();
                  Space* s = cur->clone();
                  delete cur;
                  cur = NULL;
                  path.next();
                  m.release();
                  engine().solution(s);
                }
                break;
              case SS_BRANCH:
                {
                  Space* c;
                  if ((d == 0) || (d >= engine().opt().c_d)) {
                    c = cur->clone();
                    d = 1;
                  } else {
                    c = NULL;
                    d++;
                  }
                  const Choice* ch = path.push(*this,cur,c,nid);
                  if (tracer) {
                    SearchTracer::NodeInfo ni(SearchTracer::NodeType::BRANCH,
                                              tracer.wid(), nid, *cur, ch);
                    tracer.node(ei,ni);
                  }
                  cur->commit(*ch,0);
                  m.release();
                }
                break;
              default:
                GECODE_NEVER;
              }
            }
          } else if (!path.empty()) {
            cur = path.recompute(d,engine().opt().a_d,*this,*best,mark,tracer);
            if (cur == NULL)
              path.next();
            m.release();
          } else {
            idle = true;
            path.ngdl(0);
            m.release();
            // Report that worker is idle
            engine().idle();
          }
        }
        break;
      default:
        GECODE_NEVER;
      }
    }
  }


  /*
   * Perform reset
   *
   */
  template<class Tracer>
  void
  BAB<Tracer>::reset(Space* s) {
    // Grab wait lock for reset
    m_wait_reset.acquire();
    // Release workers for reset
    release(C_RESET);
    // Wait for reset cycle started
    e_reset_ack_start.wait();
    // All workers are marked as busy again
    delete best;
    best = NULL;
    n_busy = workers();
    for (unsigned int i=1U; i<workers(); i++)
      worker(i)->reset(NULL,0);
    worker(0)->reset(s,opt().nogoods_limit);
    // Block workers again to ensure invariant
    block();
    // Release reset lock
    m_wait_reset.release();
    // Wait for reset cycle stopped
    e_reset_ack_stop.wait();
  }


  /*
   * Create no-goods
   *
   */
  template<class Tracer>
  NoGoods&
  BAB<Tracer>::nogoods(void) {
    NoGoods* ng;
    // Grab wait lock for reset
    m_wait_reset.acquire();
    // Release workers for reset
    release(C_RESET);
    // Wait for reset cycle started
    e_reset_ack_start.wait();
    ng = &worker(0)->nogoods();
    // Block workers again to ensure invariant
    block();
    // Release reset lock
    m_wait_reset.release();
    // Wait for reset cycle stopped
    e_reset_ack_stop.wait();
    return *ng;
  }

  /*
   * Termination and deletion
   */
  template<class Tracer>
  BAB<Tracer>::Worker::~Worker(void) {
    delete best;
  }

  template<class Tracer>
  BAB<Tracer>::~BAB(void) {
    terminate();
    delete best;
    heap.rfree(_worker);
  }

}}}

// STATISTICS: search-par
