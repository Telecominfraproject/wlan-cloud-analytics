//
// Created by stephane bourque on 2022-10-25.
//

#include "ALBserver.h"

#include "fmt/format.h"
#include "framework/MicroServiceFuncs.h"
#include "framework/utils.h"

namespace OpenWifi {

	void ALBRequestHandler::handleRequest([[maybe_unused]] Poco::Net::HTTPServerRequest &Request,
										  Poco::Net::HTTPServerResponse &Response) {
		Utils::SetThreadName("alb-request");
		try {
			if ((id_ % 100) == 0) {
				Logger_.debug(fmt::format("ALB-REQUEST({}): ALB Request {}.",
										  Request.clientAddress().toString(), id_));
			}
			Response.setChunkedTransferEncoding(true);
			Response.setContentType("text/html");
			Response.setDate(Poco::Timestamp());
			Response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
			Response.setKeepAlive(true);
			Response.set("Connection", "keep-alive");
			Response.setVersion(Poco::Net::HTTPMessage::HTTP_1_1);
			std::ostream &Answer = Response.send();
			Answer << ALBHealthCheckServer()->CallbackText();
		} catch (...) {
		}
	}

	ALBRequestHandlerFactory::ALBRequestHandlerFactory(Poco::Logger &L) : Logger_(L) {}

	ALBRequestHandler *
	ALBRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest &request) {
		if (request.getURI() == "/")
			return new ALBRequestHandler(Logger_, req_id_++);
		else
			return nullptr;
	}

	ALBHealthCheckServer::ALBHealthCheckServer()
		: SubSystemServer("ALBHealthCheckServer", "ALB-SVR", "alb") {}

	int ALBHealthCheckServer::Start() {
		if (MicroServiceConfigGetBool("alb.enable", false)) {
			poco_information(Logger(), "Starting...");
			Running_ = true;
			Port_ = (int)MicroServiceConfigGetInt("alb.port", 15015);
			Poco::Net::IPAddress Addr(Poco::Net::IPAddress::wildcard(
				Poco::Net::Socket::supportsIPv6() ? Poco::Net::AddressFamily::IPv6
												  : Poco::Net::AddressFamily::IPv4));
			Poco::Net::SocketAddress SockAddr(Addr, Port_);
			Poco::Net::ServerSocket ClientSocket(SockAddr, 64);

			Socket_ = std::make_unique<Poco::Net::ServerSocket>(SockAddr, Port_);
			auto Params = new Poco::Net::HTTPServerParams;
			Params->setName("ws:alb");
			Server_ = std::make_unique<Poco::Net::HTTPServer>(
				new ALBRequestHandlerFactory(Logger()), *Socket_, Params);
			Server_->start();
		}

		return 0;
	}

	void ALBHealthCheckServer::Stop() {
		poco_information(Logger(), "Stopping...");
		if (Running_)
			Server_->stopAll(true);
		poco_information(Logger(), "Stopped...");
	}

} // namespace OpenWifi