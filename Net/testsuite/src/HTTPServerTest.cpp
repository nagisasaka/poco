//
// HTTPServerTest.cpp
//
// $Id: //poco/1.3/Net/testsuite/src/HTTPServerTest.cpp#2 $
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "HTTPServerTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/StreamCopier.h"
#include <sstream>


using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::ServerSocket;
using Poco::StreamCopier;


namespace
{
	class EchoBodyRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			if (request.getChunkedTransferEncoding())
				response.setChunkedTransferEncoding(true);
			else if (request.getContentLength() != HTTPMessage::UNKNOWN_CONTENT_LENGTH)
				response.setContentLength(request.getContentLength());
			
			response.setContentType(request.getContentType());
			
			std::istream& istr = request.stream();
			std::ostream& ostr = response.send();
			int n = StreamCopier::copyStream(istr, ostr);
		}
	};
	
	class EchoHeaderRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			std::ostringstream osstr;
			request.write(osstr);
			int n = (int) osstr.str().length();
			response.setContentLength(n);
			std::ostream& ostr = response.send();
			if (request.getMethod() != HTTPRequest::HTTP_HEAD)
				request.write(ostr);
		}
	};

	class RedirectRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			response.redirect("http://www.appinf.com/");
		}
	};

	class AuthRequestHandler: public HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			response.requireAuthentication("/auth");
			response.send();
		}
	};
	
	class RequestHandlerFactory: public HTTPRequestHandlerFactory
	{
	public:
		HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
		{
			if (request.getURI() == "/echoBody")
				return new EchoBodyRequestHandler;
			else if (request.getURI() == "/echoHeader")
				return new EchoHeaderRequestHandler;
			else if (request.getURI() == "/redirect")
				return new RedirectRequestHandler();
			else if (request.getURI() == "/auth")
				return new AuthRequestHandler();
			else
				return 0;
		}
	};
}


HTTPServerTest::HTTPServerTest(const std::string& name): CppUnit::TestCase(name)
{
}


HTTPServerTest::~HTTPServerTest()
{
}


void HTTPServerTest::testIdentityRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == body.size());
	assert (response.getContentType() == "text/plain");
	assert (rbody == body);
}


void HTTPServerTest::testPutIdentityRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("PUT", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == body.size());
	assert (response.getContentType() == "text/plain");
	assert (rbody == body);
}


void HTTPServerTest::testChunkedRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assert (response.getContentType() == "text/plain");
	assert (response.getChunkedTransferEncoding());
	assert (rbody == body);
}


void HTTPServerTest::testClosedRequest()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assert (response.getContentType() == "text/plain");
	assert (!response.getChunkedTransferEncoding());
	assert (rbody == body);
}


void HTTPServerTest::testIdentityRequestKeepAlive()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(true);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	cs.setKeepAlive(true);
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == body.size());
	assert (response.getContentType() == "text/plain");
	assert (response.getKeepAlive());
	assert (rbody == body);
	
	body.assign(1000, 'y');
	request.setContentLength((int) body.length());
	request.setKeepAlive(false);
	cs.sendRequest(request) << body;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == body.size());
	assert (response.getContentType() == "text/plain");
	assert (!response.getKeepAlive());
	assert (rbody == body);}


void HTTPServerTest::testChunkedRequestKeepAlive()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(true);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	cs.setKeepAlive(true);
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody", HTTPMessage::HTTP_1_1);
	request.setContentType("text/plain");
	request.setChunkedTransferEncoding(true);
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assert (response.getContentType() == "text/plain");
	assert (response.getChunkedTransferEncoding());
	assert (rbody == body);

	body.assign(1000, 'y');
	request.setKeepAlive(false);
	cs.sendRequest(request) << body;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assert (response.getContentType() == "text/plain");
	assert (response.getChunkedTransferEncoding());
	assert (!response.getKeepAlive());
	assert (rbody == body);
}


void HTTPServerTest::testClosedRequestKeepAlive()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(true);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentType("text/plain");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == HTTPMessage::UNKNOWN_CONTENT_LENGTH);
	assert (response.getContentType() == "text/plain");
	assert (!response.getChunkedTransferEncoding());
	assert (!response.getKeepAlive());
	assert (rbody == body);
	int n = (int) rbody.size();
}


void HTTPServerTest::test100Continue()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	std::string body(5000, 'x');
	HTTPRequest request("POST", "/echoBody");
	request.setContentLength((int) body.length());
	request.setContentType("text/plain");
	request.set("Expect", "100-Continue");
	cs.sendRequest(request) << body;
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getContentLength() == body.size());
	assert (response.getContentType() == "text/plain");
	assert (rbody == body);
}


void HTTPServerTest::testRedirect()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	HTTPRequest request("GET", "/redirect");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getStatus() == HTTPResponse::HTTP_FOUND);
	assert (response.get("Location") == "http://www.appinf.com/");
	assert (rbody.empty());
}


void HTTPServerTest::testAuth()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	HTTPRequest request("GET", "/auth");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getStatus() == HTTPResponse::HTTP_UNAUTHORIZED);
	assert (response.get("WWW-Authenticate") == "Basic realm=\"/auth\"");
	assert (rbody.empty());
}


void HTTPServerTest::testNotImpl()
{
	ServerSocket svs(0);
	HTTPServerParams* pParams = new HTTPServerParams;
	pParams->setKeepAlive(false);
	HTTPServer srv(new RequestHandlerFactory, svs, pParams);
	srv.start();
	
	HTTPClientSession cs("localhost", svs.address().port());
	HTTPRequest request("GET", "/notImpl");
	cs.sendRequest(request);
	HTTPResponse response;
	std::string rbody;
	cs.receiveResponse(response) >> rbody;
	assert (response.getStatus() == HTTPResponse::HTTP_NOT_IMPLEMENTED);
	assert (rbody.empty());
}


void HTTPServerTest::setUp()
{
}


void HTTPServerTest::tearDown()
{
}


CppUnit::Test* HTTPServerTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("HTTPServerTest");

	CppUnit_addTest(pSuite, HTTPServerTest, testIdentityRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testPutIdentityRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testChunkedRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testClosedRequest);
	CppUnit_addTest(pSuite, HTTPServerTest, testIdentityRequestKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, testChunkedRequestKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, testClosedRequestKeepAlive);
	CppUnit_addTest(pSuite, HTTPServerTest, test100Continue);
	CppUnit_addTest(pSuite, HTTPServerTest, testRedirect);
	CppUnit_addTest(pSuite, HTTPServerTest, testAuth);
	CppUnit_addTest(pSuite, HTTPServerTest, testNotImpl);

	return pSuite;
}
