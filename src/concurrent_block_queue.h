#ifndef CONCURRENT_BLOCK_QUEUE_H
#define CONCURRENT_BLOCK_QUEUE_H

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include "Queue.h"

namespace Utils {

  template <typename T, size_t BLOCK_SIZE = 512> 
    requires ( std::copyable<T> || std::movable<T> )
    class ConcurrentBlockQueue {

    struct Node {
      std::vector<T> data = {};
      std::unique_ptr<Node> next = nullptr;

      Node( ) { data.reserve(BLOCK_SIZE); }
      ~Node( ) = default;
      Node( Node&& ) = default;
      Node& operator=( Node&& ) = default;

      Node( const Node& ) = delete;
      Node& operator=( const Node& ) = delete;
    };

    struct Head{
      std::unique_ptr<Node> head_block = nullptr;
      size_t block_offset = 0;
      std::mutex lock;

      T pop_data( ) {
        auto retval = head_block->data[block_offset]; 
        update_head();
        return retval;
      }

      void update_head( ) {
        ++block_offset;
        if( block_offset == BLOCK_SIZE ){
          auto next = std::move(head_block->next);
          head_block = std::move(next);

          block_offset = 0;
        }
      }

      ~Head( ){
        auto next = std::move(head_block->next);
        head_block = std::move(next);
      }

      Head( const Head& ) = delete;
      Head& operator=( const Head& ) = delete;
      Head( Head&& ) = delete;
      Head& operator=( Head&& ) = delete;
      Head( std::unique_ptr<Node> _head ) : 
        head_block(std::move( _head )), block_offset(0)
      { }

    };

    struct Tail{
      Node* tail_block = nullptr;
      size_t block_offset = 0;
      std::mutex lock;

      Tail ( ) = default;
      ~Tail( ) = default;
      Tail( const Tail& ) = delete;
      Tail& operator=( const Tail& ) = delete;
      Tail( Tail&& ) = delete;
      Tail& operator=( Tail&& ) = delete;


      void update_tail( ) {
        ++block_offset;
        if( block_offset == BLOCK_SIZE ) { 
          //tail_block->next = std::make_unique<Node>();
          auto next = std::make_unique<Node>();
          tail_block->next = std::move(next);
          tail_block = (tail_block->next).get( );
          block_offset = 0;
        }
      }

      inline void add_data( T&& val) {
        tail_block->data.emplace_back(std::move( val ));
        update_tail();
      }
    };

    Head m_head;
    Tail m_tail;
    std::atomic<size_t> m_size{0};
    std::condition_variable queue_sync;

    public:

    ConcurrentBlockQueue ( ) :
      m_head( std::move(std::make_unique<Node>( ) ) )
    {
      m_tail.tail_block = m_head.head_block.get();
      m_tail.block_offset = m_head.block_offset;
    }

    ConcurrentBlockQueue ( const ConcurrentBlockQueue& ) = delete;
    ConcurrentBlockQueue& operator= ( const ConcurrentBlockQueue& ) = delete;
    ConcurrentBlockQueue ( ConcurrentBlockQueue&& ) = delete;
    ConcurrentBlockQueue& operator= ( ConcurrentBlockQueue&& ) = delete;
    ~ConcurrentBlockQueue( ) = default;

    void enqueue( T&& val ){
      std::lock_guard<std::mutex> guard(m_tail.lock);
      m_tail.add_data(std::forward<T>( val ));
      ++m_size;
      queue_sync.notify_one();
    }

    std::optional<T> try_dequeue( ) {
      std::lock_guard<std::mutex> guard(m_head.lock);
      if( not empty() ){
        auto data = m_head.pop_data( );
        --m_size;
        return {std::move(data)};
      }

      return { };
    }

    T dequeue( ) {
      std::unique_lock<std::mutex> guard(m_head.lock);
      queue_sync.wait(guard, [this]( ){ return not empty( ); } );

      auto data = m_head.pop_data( );
      --m_size;

      return data;
    }

    bool empty( ){
      return m_size == 0;
    }

    size_t size( ) {
      return m_size;
    }

  };

  template <typename T> using ConcurrentBlockQueue_t = Utils::ConcurrentBlockQueue<T>;
} // namespace Utils

#endif // CONCURRENT_BLOCK_QUEUE_H
