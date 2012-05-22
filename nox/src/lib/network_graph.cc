#include "network_graph.hh"

namespace vigil
{
  uint64_t network::switch_port::hash_code() const
  {
    uint64_t id = dpid.as_host();
    unsigned char md[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, &port, sizeof(port));
    MD5_Update(&ctx, &id, sizeof(id));
    MD5_Final(md, &ctx);
    
    return *((uint64_t*)md);
  }

  network::hop::hop(const hop& h):
    in_switch_port(h.in_switch_port)
  {
    for (nextHops::const_iterator i = h.next_hops.begin();
	 i != h.next_hops.end(); i++)
    {
      next_hops.push_back(std::make_pair(i->first,new hop(*(i->second))));
    }
  }
}
