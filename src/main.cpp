#include "MessageProcessor.h"
#include "Queue.h"
#include "concurrent_block_queue.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>


struct Student
{
   std::string university;
   int studentId = 0;
   std::string name;

   bool operator == (const Student& )const = default;
   bool operator <=> (const Student& )const = default;

   friend std::ostream& operator << (std::ostream& out, Student stu)
   {
      out << "Name: " << stu.name << "("<< stu.studentId << ")\tUniversity: " << stu.university << "\n";
      return out;
   }
};

int main() {
   std::string names[] = {
      "Ashwin", "Agastya", "Ravi", "Anandan", "Ahilya"
      };
   std::string univs[] = {
      "IIT", "Amrita", "IISC", "NIT"
      };
   std::random_device dev;
   std::mt19937 mt(dev());
   std::uniform_int_distribution<int> dist(1, 10);
   using ThreadPool = Utils::MessageProcessor<Student, 3, Utils::WeakOrdering, Utils::ConcurrentBlockQueueT>;
   std::mutex lk;
   ThreadPool::get_instance()->setProcessor([&lk](Student val) {
         std::lock_guard<std::mutex> lock(lk);
         std::cout << std::this_thread::get_id() << ": " << val << std::endl;
         });
   ThreadPool::get_instance()->start();
   int i =0; 
   auto partitioner = [](Student& s){return s.university;};
   while(i++ < 50){
      ThreadPool::get_instance()->add(Student{univs[dist(mt)%4], dist(mt), names[dist(mt)%5]}, partitioner);
   }
   ThreadPool::get_instance()->stop();
   return 0;
}
