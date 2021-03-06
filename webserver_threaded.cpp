/**
  Uses osl/porthread and osl/webserver to make a classic 
  one-thread-per-client web server.
  
  Dr. Orion Sky Lawlor, lawlor@alaska.edu, 2012-01-03 (Public Domain)
*/
#include "webserver_threaded.h"
	
/* Service the currently connected client 
	   CAUTION: MULTITHREADED CALLS!*/
void osl::http_threaded_server::service_client(void)
{
	osl::http_served_client client=serve();
	/* FUTURE: add client authentication layer here? */
	for (unsigned int i=0;i<responders.size();i++)
		if (responders[i]->respond(client)) 
			return;
	/* else nobody wants to handle it... */
	no_responder(client);
}
void osl::http_threaded_server::no_responder(osl::http_served_client &client)
{
	client.send_error("text/html",
"<HTML><TITLE>404 Error Page</TITLE>\n"
"  <BODY><H1>404 Error Page: Not Found</H1>\n"
"	Sorry, could not find your URL \""+client.get_path()+"\".\n"
"   <P>This page generated by " __FILE__ " osl::http_threaded_server::no_responder.\n"
"  </BODY>\n"
"</HTML>");
}

/*
 Service one HTTP client, then exit.
*/
void osl_http_service_client(void *thisp)
{
	osl::http_threaded_server *thisc=(osl::http_threaded_server *)thisp;
	thisc->service_client();
}

/*
  Main loop for server, after start.  Handles clients.
*/
void osl_http_run_server(void *thisp)
{
	osl::http_threaded_server *thisc=(osl::http_threaded_server *)thisp;
	while (thisc->ready(0)) { /* here's another client--make a thread for him */
		porthread_detach(porthread_create(osl_http_service_client,thisc));
	}
}

osl::http_threaded_server::http_threaded_server(unsigned int port)
	:http_server(port) 
{ }
void osl::http_threaded_server::add_responder(http_responder *responder)
{
	responders.push_back(responder);
}

void osl::http_threaded_server::start(void) {
	server_thread=porthread_create(osl_http_run_server,this);
}

/*************** Logging code *****************/
#include <stdio.h> /* for snprintf */
#include <time.h>

#if _WIN32
#define snprintf _snprintf
#define localtime_r localtime_s
#endif

/* Logs the requests of clients, as they go by. */
bool osl::html_logger::respond(osl::http_served_client &client) {
	char ip_string[100]; skt_print_ip(ip_string,client.get_ip());

	char date_string[100]; 
	time_t t; time(&t);
	const static char *month_names[]={
		"Jan","Feb","Mar","Apr","May","Jun",
		"Jul","Aug","Sep","Oct","Nov","Dec"};
#if _WIN32 /* might use localtime_s on newer Windows compilers... */
	struct tm &lt=*localtime(&t);
#else
	struct tm lt;
	localtime_r(&t,&lt);
#endif
	snprintf(date_string,100,
		"%02d/%s/%d:%02d:%02d:%02d LOCALTIME",
		lt.tm_mday,month_names[lt.tm_mon],lt.tm_year+1900,
		lt.tm_hour,lt.tm_min,lt.tm_sec);

	out<<ip_string<<" - - ["<<date_string<<"] \"GET "<<client.get_path()<<" HTTP/1.1\" 200 1 \""<<client.get_header("Referer")<<"\" \""<<client.get_header("User-Agent")<<"\"\n";

	return false; /* we don't service clients, just log them */
}
