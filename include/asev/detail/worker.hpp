///
/// worker.hpp
///

#pragma once

#include <asev/config.hpp>
#include <asev/event_base.hpp>
#include <asev/detail/basic_thrctx.hpp>

#include <cque/mpsc_queue.hpp>

#include <chrono>
#include <vector>


namespace asev
{
namespace detail
{
/// Worker for handling events.
class worker
{
#ifdef ASEV_SYSTEM_CLOCK
  using eclipse_clock_t = std::chrono::system_clock;
#else
  using eclipse_clock_t = std::chrono::steady_clock;
#endif // ASEV_SYSTEM_CLOCK

public:
  worker(size_t worker_num, size_t index) noexcept
    : index_(index)
  {
  }

  ~worker()
  {
    while (true)
    {
      auto ev = que_.pop_unique<cque::pool_delete<event_base>>();
      if (!ev)
      {
        break;
      }
    }
  }

public:
  size_t get_index() const noexcept
  {
    return index_;
  }

  void push_event(gsl::owner<event_base*> ev) noexcept
  {
    que_.push(ev);
  }

  template <typename EvService>
  size_t work(basic_thrctx<EvService>& thrctx) noexcept
  {
    size_t works = 0;
    /// Event loop.
    while (true)
    {
      auto ev = que_.pop();
      if (ev == nullptr)
      {
        /// @todo improve loop strategy.
        break;
      }

      bool is_auto = true;
      try
      {
        ++works;
        is_auto = ev->handle(thrctx);
      }
      catch (...)
      {
        Ensures(false);
      }

      if (is_auto)
      {
        ev->release();
      }
    }
    return works;
  }

private:
  size_t index_;
  cque::mpsc_queue<event_base, eclipse_clock_t> que_;
};
}
}
