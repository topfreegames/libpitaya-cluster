#include <etcd/v3/Action.hpp>


etcdv3::Action::Action(ActionParameters params)
  : parameters(std::move(params))
{}

void etcdv3::Action::waitForResponse()
{
  void * got_tag;
  bool ok = false;

  if (cq_.Next(&got_tag, &ok)) {
    if (got_tag != (void *)this) {
      throw std::runtime_error("Assertion error: `got_tag == (void*)this`");
    }
  }
}

