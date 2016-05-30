/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_MIXIN_REQUESTER_HPP
#define CAF_MIXIN_REQUESTER_HPP

#include <tuple>
#include <chrono>

#include "caf/fwd.hpp"
#include "caf/actor.hpp"
#include "caf/message.hpp"
#include "caf/duration.hpp"
#include "caf/response_handle.hpp"
#include "caf/message_priority.hpp"
#include "caf/check_typed_input.hpp"

namespace caf {
namespace mixin {

template <class T>
struct is_blocking_requester : std::false_type { };

/// A `requester` is an actor that supports
/// `self->request(...).{then|await|receive}`.
template <class Base, class Subtype>
class requester : public Base {
public:
  template <class... Ts>
  requester(Ts&&... xs) : Base(std::forward<Ts>(xs)...) {
    // nop
  }

  static constexpr bool is_blocking_subtype = is_blocking_requester<Subtype>::value;

  /// Sends `{xs...}` as a synchronous message to `dest` with priority `mp`.
  /// @returns A handle identifying a future-like handle to the response.
  /// @warning The returned handle is actor specific and the response to the
  ///          sent message cannot be received by another actor.
  template <message_priority P = message_priority::normal,
            class Handle = actor, class... Ts>
  response_handle<Subtype,
                  typename detail::deduce_output_type<
                    Handle,
                    detail::type_list<
                      typename detail::implicit_conversions<
                        typename std::decay<Ts>::type
                      >::type...>
                  >::type,
                  is_blocking_subtype>
  request(const Handle& dest, const duration& timeout, Ts&&... xs) {
    static_assert(sizeof...(Ts) > 0, "no message to send");
    using token =
      detail::type_list<
        typename detail::implicit_conversions<
          typename std::decay<Ts>::type
        >::type...>;
    static_assert(actor_accepts_message(signatures_of<Handle>(), token{}),
                  "receiver does not accept given message");
    auto req_id = dptr()->new_request_id(P);
    dest->eq_impl(req_id, dptr()->ctrl(), dptr()->context(),
                  std::forward<Ts>(xs)...);
    dptr()->request_sync_timeout_msg(timeout, req_id);
    return {req_id.response_id(), dptr()};
  }

private:
  Subtype* dptr() {
    return static_cast<Subtype*>(this);
  }
};

} // namespace mixin
} // namespace caf

#endif // CAF_MIXIN_REQUESTER_HPP