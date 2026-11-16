#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ets2la_plugin
{
    class ThreadPool
    {
    private:
        std::vector< std::thread > _workers;
        std::queue< std::function< void() > > _tasks;
        std::mutex _queueMutex;
        std::condition_variable _cvWork;
        std::condition_variable _cvDone;
        std::atomic< size_t > _activeTasks{ 0 };
        std::atomic< bool > _stopping{ false };

        void WorkerLoop()
        {
            while ( true )
            {
                std::function< void() > task;
                {
                    std::unique_lock< std::mutex > lock( _queueMutex );
                    _cvWork.wait( lock, [ this ]()
                    {
                        return _stopping.load( std::memory_order_relaxed ) || !_tasks.empty();
                    } );

                    if ( _stopping.load( std::memory_order_relaxed ) && _tasks.empty() )
                    {
                        return;
                    }

                    task = std::move( _tasks.front() );
                    _tasks.pop();
                    _activeTasks.fetch_add( 1, std::memory_order_relaxed );
                }

                task();

                {
                    std::unique_lock< std::mutex > lock( _queueMutex );
                    const size_t prev = _activeTasks.fetch_sub( 1, std::memory_order_release );
                    if ( prev == 1 && _tasks.empty() )
                    {
                        _cvDone.notify_all();
                    }
                }
            }
        }

    public:
        explicit ThreadPool( size_t numThreads )
        {
            _workers.reserve( numThreads );
            for ( size_t i = 0; i < numThreads; ++i )
            {
                _workers.emplace_back( &ThreadPool::WorkerLoop, this );
            }
        }

        ~ThreadPool()
        {
            {
                std::unique_lock< std::mutex > lock( _queueMutex );
                _stopping.store( true, std::memory_order_relaxed );
            }
            _cvWork.notify_all();

            for ( auto& worker : _workers )
            {
                if ( worker.joinable() )
                {
                    worker.join();
                }
            }
        }

        void Enqueue( std::function< void() > task )
        {
            {
                std::unique_lock< std::mutex > lock( _queueMutex );
                _tasks.push( std::move( task ) );
            }
            _cvWork.notify_one();
        }

        void WaitForAll()
        {
            std::unique_lock< std::mutex > lock( _queueMutex );
            _cvDone.wait( lock, [ this ]()
            {
                return _tasks.empty() && _activeTasks.load( std::memory_order_acquire ) == 0;
            } );
        }

        size_t ThreadCount() const
        {
            return _workers.size();
        }
    };
}
