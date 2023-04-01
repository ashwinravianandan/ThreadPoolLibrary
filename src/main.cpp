#include "MessageProcessor.h"
#include "Queue.h"
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
   using ThreadPool = Utils::MessageProcessor<std::string, 10>;

   ThreadPool::get_instance()->setProcessor([](const std::string& val)
         {
         std::cout << std::this_thread::get_id() << ": \t" << val << std::endl;
         });

   ThreadPool::get_instance()->start();
   ThreadPool::get_instance()->add("Hello World");
   ThreadPool::get_instance()->stop();
   return 0;
}
