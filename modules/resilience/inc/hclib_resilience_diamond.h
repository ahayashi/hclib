#ifndef HCLIB_RESILIENCE_DIAMOND_H
#define HCLIB_RESILIENCE_DIAMOND_H

#define N_CNT 3

namespace ref_count = hclib::ref_count;

namespace hclib {
namespace resilience {
namespace diamond {

bool is_diamond_task(void *ptr)
{
    return nullptr != ptr;
}

/*
Reslient Future and Promise for pointer type data
*/
template<typename T>
class future_t: public ref_count::future_t<T> {
  public:
    void ref_count_decr();
    void add_future_vector();

    void release() {
      void *ptr = * hclib_get_curr_task_local();
      if(is_diamond_task(ptr)){
        add_future_vector();
      }
      else {
        ref_count_decr();
      }
    }
};

//Currently only pointer type promise is allowed
template<typename T>
class promise_t: public ref_count::promise_t<T> {
    promise_t() = delete;
    promise_t(const promise_t &) = delete;
    promise_t(promise_t &&) = delete;
};


//TODO: To enable non pointer and reference types, 
//create template specialisation for pointer and reference
//and get the required data pointer as given in default impl.
//and store that data to tmp_data.
//the specialisations only override put().
//put_acutal() remains same
template<typename T>
class promise_t<T*>: public ref_count::promise_t<T*> {

    static const int TYPE = ref_count::promise_t<T*>::TYPE+1;

    //replica i stores datum in tmp_data[i] when put is performed
    //and later at the end of diamond task it performs actual put
    T* tmp_data[N_CNT] = {nullptr};

  public:

    promise_t(int ref_cnt, ref_count::DelType del_type = ref_count::DelType::NORMAL)
    : ref_count::promise_t<T*>(ref_cnt, del_type) {
          hclib_promise_t::type = TYPE;
    }

    bool equal(int i, int j) {
        obj* obj1 = static_cast<obj*>(tmp_data[i]);
        obj* obj2 = static_cast<obj*>(tmp_data[j]);

        //Each replica should have allocated its own data
        //otherwise all replicas are modifying the same data
        assert(obj1 != obj2);
        return obj1->equals(obj2);
    }

    void put(T* datum);

    future_t<T*> *get_future() {
        return static_cast<future_t<T*>*>( hclib::promise_t<T*>::get_future());
    }

    void put_actual(int replica_index) {
        for(int i=0; i<N_CNT; i++)
          if( i!=replica_index )
             (* ref_count::promise_t<T*>::deleter)(tmp_data[i]);
        hclib::promise_t<T*>::put(tmp_data[replica_index]);
    }
};

/*
safe vector of promises and futures
*/
template<typename T>
using promise_vector = hclib::resilience::util::safe_vector<promise_t<T>*>;

template<typename T>
using future_vector = hclib::resilience::util::safe_vector<future_t<T>*>;

/*
metadata that is passed between tasks created within each replica

All put operations should be added to a list, so that they
can be later compared.
All futures that are used as task dependencies should be
added to a list, so that they can be later released.
*/
template<typename T>
struct diamond_task_params_t {
    int index; //replica id

    //vector which captures all the puts
    promise_vector<T> *put_vec;

    //vector which capture task dependencies
    future_vector<T> *rel_vec;
};

/*
definition of future_t and promise_t functions
*/
template<typename T>
void future_t<T>::ref_count_decr() {
    auto p = static_cast<promise_t<T>*>(hclib_future_t::owner);
    p->ref_count_decr();
}

template<typename T>
void future_t<T>::add_future_vector() {
    auto task_local = static_cast<diamond_task_params_t<T>*>(*hclib_get_curr_task_local());
    assert(is_diamond_task(task_local));
    if(task_local->index==0)
        task_local->rel_vec->push_back(this);
}

/*
put operation only adds data to a temporary storage indexed 
by its replica id. And the first replica adds the promise 
to the vector later used to perform the actual put
*/
template<typename T>
void promise_t<T*>::put(T* datum) {
    //HASSERT_STATIC(std::is_pointer<T*>::value,
    //    "resilient promise_t currently only supports pointers\n");
    auto task_local = static_cast<diamond_task_params_t<T*>*>(*hclib_get_curr_task_local());

    //if called from diamond task, save to a temp and delay the put
    //also add it to the vector that captures all put opertations
    if(is_diamond_task(task_local)){
        tmp_data[task_local->index] = datum;
        if(task_local->index==0)
            task_local->put_vec->push_back(this);
    }
    //if called from non-diamond task, just perform the put
    else {
        hclib::promise_t<T*>::put(datum);
    }
}

/*
Resilient async_await
*/

template<typename T>
bool check_result_helper(promise_vector<T>* put_vec,
        int replica1, int replica2) {

    for(auto && elem: *put_vec) {
        if(!elem->equal(replica1, replica2))
            return false;
    }
    return true;
}

//TODO: currently only works for triple redundancy
template<typename T>
int check_result(promise_vector<T>* put_vec) {
    if(check_result_helper(put_vec, 0, 1))
        return 0;
    else if(check_result_helper(put_vec, 1, 2))
        return 1;
    else if(check_result_helper(put_vec, 0, 2))
        return 0;
    else
        return -1;
}

int get_replica_index() {
  auto task_local = static_cast<diamond_task_params_t<void*>*>(*hclib_get_curr_task_local());
  assert(is_diamond_task(task_local));
  return task_local->index;
}

template <typename T>
void async_await(T&& lambda, hclib_future_t *future1,
        hclib_future_t *future2=nullptr, hclib_future_t *future3=nullptr,
        hclib_future_t *future4=nullptr) {

    //fetch the diamond_task_parameters from task local and pass is to the async
    auto dtp = *hclib_get_curr_task_local();

    hclib::async_await( [=, lambda_mv = std::move(lambda)] () {
        *(hclib_get_curr_task_local()) = dtp;
	lambda_mv();
        auto task_local = static_cast<diamond_task_params_t<void*>*>(*hclib_get_curr_task_local());
        //if(task_local->index == 0) {
        if(future1 != nullptr)
            //static_cast<future_t<void*>*>(future1)->release();
            static_cast<future_t<void*>*>(future1)->add_future_vector();
        if(future2 != nullptr)
            //static_cast<future_t<void*>*>(future2)->release();
            static_cast<future_t<void*>*>(future2)->add_future_vector();
        if(future3 != nullptr)
            //static_cast<future_t<void*>*>(future3)->release();
            static_cast<future_t<void*>*>(future3)->add_future_vector();
        if(future4 != nullptr)
            //static_cast<future_t<void*>*>(future4)->release();
            static_cast<future_t<void*>*>(future4)->add_future_vector();
        //}
    }, future1, future2, future3, future4);
}

} // namespace diamond
} // namespace resilience
} // namespace hclib

#include "hclib_resilience_diamond2.h"
#include "hclib_resilience_diamond3.h"

#endif
