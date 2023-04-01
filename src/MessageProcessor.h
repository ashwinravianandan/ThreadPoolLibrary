#ifndef _MESSAGE_PROCESSOR_H_
#define _MESSAGE_PROCESSOR_H_
#include "Queue.h"
#include <exception>
#include <functional>
#include <stop_token>
#include <thread>
#include <iostream>
#include "Singleton.h"

namespace Utils
{
   template <typename T>
      using FIFOSyncQueue = SyncronizedQueue<FIFOQueue, T>;

   template <typename T>
      concept Equal = requires (T&& a, T&& b)
      {a == b;};

   template <typename T> requires (std::copyable<T> || std::movable<T> ) && Equal<T>
      struct Sentinel
      {
         static T getValue()
         {
            // default constructed object is used as sentinel
            return T{};
         }
      };


   template < typename T, int NoOfConsumers = 1, template <typename> class Q = FIFOSyncQueue>
      requires Queue<Q<T>, T>
      class MessageProcessor: public Singleton<MessageProcessor<T,NoOfConsumers, Q>>
      {
         friend Singleton<MessageProcessor<T,NoOfConsumers, Q>>;
         private:
            Q<T> _queue;
            std::function<void(T&&)> _processor;
            std::jthread _threads[NoOfConsumers];
            bool _stopRequested;
            void run(std::stop_token& token)
            {
               auto sentinel = Sentinel<T>::getValue();
               while(!token.stop_requested())
               {
                  try{
                     auto val = _queue.dequeue();
                     if(sentinel != val){
                        _processor(std::move(val));
                     }
                     else
                        break;
                  }
                  catch(std::exception& ex)
                  {
                     std::cout << "Error occurred while processing message: " << ex.what();
                  }
                  catch(...)
                  {
                     std::cout << "Unknown error occurred while processing message\n";
                  }
               }
            }
            MessageProcessor() = default;
         public:

            ~MessageProcessor() = default;

            template<typename V>
            void add(V&& data) requires std::convertible_to<V, T>
            {
               if(!_stopRequested){
                  _queue.enqueue(T(std::forward<V>(data)));
               }
            }

            void setProcessor(const std::function<void(T&&)>& processor)
            {
               _processor = processor;
            }
            void start()
            {
               _stopRequested = false;
               for(int i = 0; i< NoOfConsumers; i++)
               {
                  _threads[i] = std::jthread(
                        [this](std::stop_token token){
                        run(token);
                        });
               }
            }
            void stop()
            {
               _stopRequested = true;
               for(int i = 0; i< NoOfConsumers; i++)
               {
                  _threads[i].request_stop();
                  _queue.enqueue(Sentinel<T>::getValue());
               }
            }
      };
}
#endif
