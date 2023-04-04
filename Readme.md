# Introduction
This template library implements an extendable thread pool framework in C++20.
The `MessageProcessor` type can be instantiated in multiple configurations to affect the behaviour
of the thread pool.  It is possible to implement ones own custom Queue by adhering to the policy defined in *Queue.h*

The client code must assign a functor that represents the operation to be performed on the data enqueued.
Any shared resource used within this functor would have to be protected against multi-threaded access.

```cpp
   // A simple processor that prints enqueued data to the console
   ThreadPool::get_instance()->setProcessor([&lk](Student val) {
         std::lock_guard<std::mutex> lock(lk);
         std::cout << std::this_thread::get_id() << ": " << val << std::endl;
         });
```

# Configurations
Here are some sample configurations of the thread pool

## Thread-safe, unordered, enqueue(non-blocking)
### Instantiation
The below snippet instantiates a thread pool that would process `std::string` payloads
using a pool of 8 threads.  The `enqueue` method would queue a request that may be processed by any of the 
8 threads that is available at the moment

```cpp
using ThreadPool = Utils::MessageProcessor<std::string, 8>;
```


## Thread-safe, unordered, enqueue(blocking)
### Instantiation
The below snippet instantiates a thread pool that would process `std::string` payloads
using a pool of 8 threads.  The `enqueue` method would **block** if all threads in the pool have work
queued up for them.  It would wait for a slot to become available to enqueue work.  The approach can be 
used to apply back-pressure on the producer.

```cpp
using ThreadPool = Utils::MessageProcessor<std::string, 8, Utils::NoOrdering, Utils::NonBufferingQueue>;
```

## Thread-safe, weakly ordered, enqueue(non-blocking)
### Instantiation
The below snippet instantiates a thread pool that would process `Student` payloads
using a pool of 3 threads.  The `add` method expects an additional callable parameter that operates on 
the `Student` type to extract an ID field for partitioning.  Two *items* having the same ID will be processed in
sequence but those that don't same the same ID could be processed in between.'

```cpp
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

using ThreadPool = Utils::MessageProcessor<Student, 3, Utils::WeakOrdering>;
```

The below output illustrates this.  Student processing is partitioned by University by invoking add as show below:

```cpp
   auto partitioner = [](Student& s){return s.university;};
   while(i++ < 50){
      ThreadPool::get_instance()->add(Student{univs[dist(mt)%4], dist(mt), names[dist(mt)%5]}, partitioner);
   }
```

#### Output

```
139700855584512: Name: Ravi(3)	University: Amrita

139700855584512: Name: Agastya(4)	University: Amrita

139700855584512: Name: Anandan(6)	University: Amrita

139700855584512: Name: Ravi(8)	University: Amrita

139700855584512: Name: Anandan(2)	University: Amrita

139700855584512: Name: Ravi(4)	University: Amrita

139700855584512: Name: Ahilya(6)	University: Amrita

139700855584512: Name: Ahilya(7)	University: Amrita

139700855584512: Name: Ashwin(1)	University: Amrita

139700855584512: Name: Ahilya(7)	University: Amrita

139700855584512: Name: Anandan(2)	University: Amrita

139700855584512: Name: Anandan(5)	University: Amrita

139700855584512: Name: Anandan(8)	University: Amrita

139700855584512: Name: Ravi(3)	University: Amrita

139700855584512: Name: Ahilya(1)	University: Amrita

139700872369920: Name: Ravi(6)	University: IISC

139700872369920: Name: Ravi(1)	University: IISC

139700872369920: Name: Ravi(7)	University: IISC

139700872369920: Name: Ravi(2)	University: IISC

139700872369920: Name: Anandan(7)	University: IISC

139700872369920: Name: Ahilya(4)	University: IISC

139700872369920: Name: Ravi(6)	University: IISC

139700872369920: Name: Ashwin(5)	University: IISC

139700872369920: Name: Ahilya(9)	University: IISC

139700872369920: Name: Agastya(10)	University: IISC

139700872369920: Name: Ashwin(10)	University: IISC

139700863977216: Name: Agastya(1)	University: IIT

139700863977216: Name: Ashwin(8)	University: IIT

139700863977216: Name: Agastya(8)	University: NIT

139700863977216: Name: Ahilya(6)	University: IIT

139700863977216: Name: Ashwin(7)	University: IIT

139700863977216: Name: Ahilya(10)	University: NIT

139700872369920: Name: Anandan(3)	University: IISC

139700863977216: Name: Ashwin(1)	University: IIT

139700863977216: Name: Ahilya(2)	University: NIT

139700863977216: Name: Agastya(5)	University: NIT

139700863977216: Name: Ashwin(5)	University: NIT

139700863977216: Name: Ravi(5)	University: NIT

139700863977216: Name: Ahilya(10)	University: IIT

139700863977216: Name: Ravi(9)	University: IIT

139700863977216: Name: Agastya(8)	University: NIT

139700863977216: Name: Ravi(7)	University: IIT

139700863977216: Name: Ravi(3)	University: NIT

139700863977216: Name: Agastya(4)	University: NIT

139700863977216: Name: Anandan(9)	University: IIT

139700863977216: Name: Ravi(10)	University: IIT

139700863977216: Name: Anandan(8)	University: IIT

139700863977216: Name: Ashwin(6)	University: IIT

139700863977216: Name: Ashwin(4)	University: NIT

139700863977216: Name: Ashwin(9)	University: NIT
```
