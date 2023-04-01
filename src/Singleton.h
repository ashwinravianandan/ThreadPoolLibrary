#include <memory>
#include <mutex>
#include <thread>

namespace Utils
{
   template<typename T>
      class Singleton
      {
         using Derived = T;
         using InstancePtr = std::shared_ptr<T>;
         private:
         static InstancePtr _instance;
         static std::once_flag _once;
         public:
         static InstancePtr get_instance()
         {
            if(!_instance)
            {
               std::call_once(_once, [](){_instance = std::shared_ptr<T>(new Derived);});
            }
            return _instance;
         }
      };

   template <typename T>
      std::once_flag Singleton<T>::_once;

   template <typename T>
      std::shared_ptr<T> Singleton<T>::_instance;
}
