/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
 
   This class isn't in use any more.
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


package freeciv.servlet;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Map;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.commons.httpclient.Header;
import org.apache.commons.httpclient.HttpClient;
import org.apache.commons.httpclient.HttpMethod;
import org.apache.commons.httpclient.NameValuePair;
import org.apache.commons.httpclient.methods.GetMethod;
import org.apache.commons.httpclient.methods.PostMethod;
import org.apache.commons.httpclient.params.*;
import org.apache.commons.httpclient.MultiThreadedHttpConnectionManager;
import org.apache.commons.httpclient.HttpException;


public class ProxyServlet extends HttpServlet {
	/**
	 * Serialization UID.
	 */
	private static final long serialVersionUID = 1L;
	/**
	 * Key for redirect location header.
	 */
    private static final String STRING_LOCATION_HEADER = "Location";
    /**
     * Key for content type header.
     */
    private static final String STRING_CONTENT_TYPE_HEADER_NAME = "Content-Type";

    /**
     * Key for content length header.
     */
    private static final String STRING_CONTENT_LENGTH_HEADER_NAME = "Content-Length";
    /**
     * Key for host header
     */
    private static final String STRING_HOST_HEADER_NAME = "Host";

    
    // Proxy host params
    /**
     * The host to which we are proxying requests
     */
	private String stringProxyHost;
	/**
	 * The port on the proxy host to wihch we are proxying requests. Default value is 80.
	 */
	private int intProxyPort = 8081;
	
	/** If lockedProxyPort is true, then the proxy port has been set
	 *  from the servlet-engine in web.xml, and shoudn't be changed later.
	 */ 
	private boolean lockedProxyPort = false;
	/**
	 * The (optional) path on the proxy host to wihch we are proxying requests. Default value is "".
	 */
	private String stringProxyPath = "";
	
	// Must use MultiThreadedHttpConnectionManager because
	// there can me multiple clients (AJAX HTTP clients).
	static MultiThreadedHttpConnectionManager connectionManager = 
  		new MultiThreadedHttpConnectionManager();
  	static HttpClient client = new HttpClient(connectionManager);

	
	/**
	 * Initialize the <code>ProxyServlet</code>
	 * @param servletConfig The Servlet configuration passed in by the servlet conatiner
	 */
	public void init(ServletConfig servletConfig) {
		// Get the proxy host
		String stringProxyHostNew = servletConfig.getInitParameter("proxyHost");
		if(stringProxyHostNew == null || stringProxyHostNew.length() == 0) { 
			throw new IllegalArgumentException("Proxy host not set, please set init-param 'proxyHost' in web.xml");
		}
		this.setProxyHost(stringProxyHostNew);
		// Get the proxy path if specified
		String stringProxyPathNew = servletConfig.getInitParameter("proxyPath");
		if(stringProxyPathNew != null && stringProxyPathNew.length() > 0) {
			this.setProxyPath(stringProxyPathNew);
		}
		// Get the maximum file upload size if specified
		String stringMaxFileUploadSize = servletConfig.getInitParameter("maxFileUploadSize");

		

		
	}
	
	/**
	 * Performs an HTTP GET request
	 * @param httpServletRequest The {@link HttpServletRequest} object passed
	 *                            in by the servlet engine representing the
	 *                            client request to be proxied
	 * @param httpServletResponse The {@link HttpServletResponse} object by which
	 *                             we can send a proxied response to the client 
	 */
	public void doGet (HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse)
    		throws IOException, ServletException {

		// Create a GET request
		GetMethod getMethodProxyRequest = new GetMethod(this.getProxyURL(httpServletRequest));
		// Forward the request headers
		setProxyRequestHeaders(httpServletRequest, getMethodProxyRequest);
	   	
		// Ensures that Web browsers / Ajax clients don't cache the XML.
		httpServletResponse.setHeader("Cache-Control","no-cache"); //HTTP 1.1
    	httpServletResponse.setHeader("Pragma","no-cache"); //HTTP 1.0
    	httpServletResponse.setDateHeader ("Expires", 0); //prevents caching at the proxy server
		
    	// Execute the proxy request
		this.executeProxyRequest(getMethodProxyRequest, httpServletRequest, httpServletResponse);
	}
	
	/**
	 * Performs an HTTP POST request
	 * @param httpServletRequest The {@link HttpServletRequest} object passed
	 *                            in by the servlet engine representing the
	 *                            client request to be proxied
	 * @param httpServletResponse The {@link HttpServletResponse} object by which
	 *                             we can send a proxied response to the client 
	 */
	public void doPost(HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse)
        	throws IOException, ServletException {
		
		// Create a standard POST request
    	PostMethod postMethodProxyRequest = new PostMethod(this.getProxyURL(httpServletRequest));
		// Forward the request headers
		setProxyRequestHeaders(httpServletRequest, postMethodProxyRequest);
		// don't change Content-type header from client.
		//postMethodProxyRequest.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");

		postMethodProxyRequest.setRequestBody(httpServletRequest.getInputStream());
		
    	this.executeProxyRequest(postMethodProxyRequest, httpServletRequest, httpServletResponse);
	    httpServletResponse.setHeader("Cache-Control","no-cache"); //HTTP 1.1
	    httpServletResponse.setHeader("Pragma","no-cache"); //HTTP 1.0
	    httpServletResponse.setDateHeader ("Expires", 0); //prevents caching at the proxy server
    }
	
		

 
    
    /**
     * Executes the {@link HttpMethod} passed in and sends the proxy response
     * back to the client via the given {@link HttpServletResponse}
     * @param httpMethodProxyRequest An object representing the proxy request to be made
     * @param httpServletResponse An object by which we can send the proxied
     *                             response back to the client
     * @throws IOException Can be thrown by the {@link HttpClient}.executeMethod
     * @throws ServletException Can be thrown to indicate that another error has occurred
     */
    private void executeProxyRequest(
    		HttpMethod httpMethodProxyRequest,
    		HttpServletRequest httpServletRequest,
    		HttpServletResponse httpServletResponse)
    			throws IOException, ServletException {


    	
    	httpMethodProxyRequest.setFollowRedirects(false);
        String port = "" + httpServletRequest.getSession().getAttribute("civserverport");
        String host = "" + httpServletRequest.getSession().getAttribute("civserverhost");
        String username = "" + httpServletRequest.getSession().getAttribute("username");
        httpMethodProxyRequest.addRequestHeader("civserverport", port);
        httpMethodProxyRequest.addRequestHeader("civserverhost", host);
        httpMethodProxyRequest.addRequestHeader("username", username);
    	int intProxyResponseCode = 0;
    	// Execute the request
    	try {
    		intProxyResponseCode = client.executeMethod(httpMethodProxyRequest);
    	} catch (IOException ioErr) {
    		//- If an I/O (transport) error occurs. Some transport exceptions can be recovered from. 
    		//- If a protocol exception occurs. Usually protocol exceptions cannot be recovered from.
    		OutputStream outputStreamClientResponse = httpServletResponse.getOutputStream();
    		httpServletResponse.setStatus(502);
            outputStreamClientResponse.write("Freeciv web client proxy not responding (most likely died).".getBytes());
            httpMethodProxyRequest.releaseConnection();
            return;
    	} 

		// Check if the proxy response is a redirect
		// The following code is adapted from org.tigris.noodle.filters.CheckForRedirect
		// Hooray for open source software
		if(intProxyResponseCode >= HttpServletResponse.SC_MULTIPLE_CHOICES /* 300 */
				&& intProxyResponseCode < HttpServletResponse.SC_NOT_MODIFIED /* 304 */) {
			String stringStatusCode = Integer.toString(intProxyResponseCode);
			String stringLocation = httpMethodProxyRequest.getResponseHeader(STRING_LOCATION_HEADER).getValue();
			if(stringLocation == null) {
					httpMethodProxyRequest.releaseConnection();
					throw new ServletException("Recieved status code: " + stringStatusCode 
							+ " but no " +  STRING_LOCATION_HEADER + " header was found in the response");
			}
			// Modify the redirect to go to this proxy servlet rather that the proxied host
			String stringMyHostName = httpServletRequest.getServerName();
			if(httpServletRequest.getServerPort() != 80) {
				stringMyHostName += ":" + httpServletRequest.getServerPort();
			}
			stringMyHostName += httpServletRequest.getContextPath();
			httpServletResponse.sendRedirect(stringLocation.replace(getProxyHostAndPort() + this.getProxyPath(), stringMyHostName));
			httpMethodProxyRequest.releaseConnection();
			return;
		} else if(intProxyResponseCode == HttpServletResponse.SC_NOT_MODIFIED) {
			// 304 needs special handling.  See:
			// http://www.ics.uci.edu/pub/ietf/http/rfc1945.html#Code304
			// We get a 304 whenever passed an 'If-Modified-Since'
			// header and the data on disk has not changed; server
			// responds w/ a 304 saying I'm not going to send the
			// body because the file has not changed.
			httpServletResponse.setIntHeader(STRING_CONTENT_LENGTH_HEADER_NAME, 0);
			httpServletResponse.setStatus(HttpServletResponse.SC_NOT_MODIFIED);
			httpMethodProxyRequest.releaseConnection();
			return;
		}
		
		// Pass the response code back to the client
		httpServletResponse.setStatus(intProxyResponseCode);

        // Pass response headers back to the client
        Header[] headerArrayResponse = httpMethodProxyRequest.getResponseHeaders();
        for(Header header : headerArrayResponse) {
       		httpServletResponse.setHeader(header.getName(), header.getValue());
        }
        
        // Send the content to the client
        InputStream inputStreamProxyResponse = httpMethodProxyRequest.getResponseBodyAsStream();
        BufferedInputStream bufferedInputStream = new BufferedInputStream(inputStreamProxyResponse);
        OutputStream outputStreamClientResponse = httpServletResponse.getOutputStream();
        int intNextByte;
        while ( ( intNextByte = bufferedInputStream.read() ) != -1 ) {
        	outputStreamClientResponse.write(intNextByte);
        }
        httpMethodProxyRequest.releaseConnection();
    }
    
    public String getServletInfo() {
        return "Freeciv Proxy Servlet";
    }

    /**
     * Retreives all of the headers from the servlet request and sets them on
     * the proxy request
     * 
     * @param httpServletRequest The request object representing the client's
     *                            request to the servlet engine
     * @param httpMethodProxyRequest The request that we are about to send to
     *                                the proxy host
     */
    @SuppressWarnings("unchecked")
	private void setProxyRequestHeaders(HttpServletRequest httpServletRequest, HttpMethod httpMethodProxyRequest) {
    	// Get an Enumeration of all of the header names sent by the client
		Enumeration enumerationOfHeaderNames = httpServletRequest.getHeaderNames();
		while(enumerationOfHeaderNames.hasMoreElements()) {
			String stringHeaderName = (String) enumerationOfHeaderNames.nextElement();
			if(stringHeaderName.equalsIgnoreCase(STRING_CONTENT_LENGTH_HEADER_NAME))
				continue;
			// As per the Java Servlet API 2.5 documentation:
			//		Some headers, such as Accept-Language can be sent by clients
			//		as several headers each with a different value rather than
			//		sending the header as a comma separated list.
			// Thus, we get an Enumeration of the header values sent by the client
			Enumeration enumerationOfHeaderValues = httpServletRequest.getHeaders(stringHeaderName);
			while(enumerationOfHeaderValues.hasMoreElements()) {
				String stringHeaderValue = (String) enumerationOfHeaderValues.nextElement();
				// In case the proxy host is running multiple virtual servers,
				// rewrite the Host header to ensure that we get content from
				// the correct virtual server
				if(stringHeaderName.equalsIgnoreCase(STRING_HOST_HEADER_NAME)){
					stringHeaderValue = getProxyHostAndPort();
				}
				Header header = new Header(stringHeaderName, stringHeaderValue);
				// Set the same header on the proxy request
				httpMethodProxyRequest.setRequestHeader(header);
			}
		}
    }
    
	// Accessors
    private String getProxyURL(HttpServletRequest httpServletRequest) {
		// Set the protocol to HTTP
		String stringProxyURL = "http://" + this.getProxyHostAndPort();
		// Check if we are proxying to a path other that the document root
		if(!this.getProxyPath().equalsIgnoreCase("")){
			stringProxyURL += this.getProxyPath();
		}
		return stringProxyURL;
    }
    
    private String getProxyHostAndPort() {
    	if(this.getProxyPort() == 80) {
    		return this.getProxyHost();
    	} else {
    		return this.getProxyHost() + ":" + this.getProxyPort();
    	}
	}
    
	private String getProxyHost() {
		return this.stringProxyHost;
	}
	private void setProxyHost(String stringProxyHostNew) {
		this.stringProxyHost = stringProxyHostNew;
	}
	private int getProxyPort() {
		return this.intProxyPort;
	}
	private void setProxyPort(int intProxyPortNew) {
		this.intProxyPort = intProxyPortNew;
		this.lockedProxyPort = true;
	}
	private String getProxyPath() {
		return this.stringProxyPath;
	}
	private void setProxyPath(String stringProxyPathNew) {
		this.stringProxyPath = stringProxyPathNew;
	}

}
