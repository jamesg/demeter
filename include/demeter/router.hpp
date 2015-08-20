#ifndef DEMETER_ROUTER_HPP
#define DEMETER_ROUTER_HPP

#include <boost/shared_ptr.hpp>

#include "atlas/http/server/application_router.hpp"
#include "atlas/http/server/mimetypes.hpp"

namespace hades
{
    class connection;
}
namespace demeter
{
    class router : public atlas::http::application_router
    {
    public:
        router(boost::shared_ptr<boost::asio::io_service>, hades::connection&);
    };
}

#endif
