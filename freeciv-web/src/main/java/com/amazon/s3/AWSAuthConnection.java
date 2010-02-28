/********************************************************************** 
 Freeciv - Copyright (C) 2010 - Andreas Røsdal   andrearo@pvv.ntnu.no
 
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

//  This software code is made available "AS IS" without warranties of any
//  kind.  You may copy, display, modify and redistribute the software
//  code either by itself or as incorporated into your code; provided that
//  you do not remove any proprietary notices.  Your use of this software
//  code is at your own risk and you waive any claim against Amazon
//  Digital Services, Inc. or its affiliates with respect to your use of
//  this software code. (c) 2006-2007 Amazon Digital Services, Inc. or its
//  affiliates.

package com.amazon.s3;

import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.io.IOException;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;

/**
 * An interface into the S3 system.  It is initially configured with
 * authentication and connection parameters and exposes methods to access and
 * manipulate S3 data.
 */
public class AWSAuthConnection {
    public static final String LOCATION_DEFAULT = null;
    public static final String LOCATION_EU = "EU";

    private String awsAccessKeyId;
    private String awsSecretAccessKey;
    private boolean isSecure;
    private String server;
    private int port;
    private CallingFormat callingFormat;

    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey) {
        this(awsAccessKeyId, awsSecretAccessKey, true);
    }

    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey, boolean isSecure) {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, Utils.DEFAULT_HOST);
    }

    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey, boolean isSecure,
                             String server)
    {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server,
             isSecure ? Utils.SECURE_PORT : Utils.INSECURE_PORT);
    }

    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey, boolean isSecure, 
                             String server, int port) {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server, port, CallingFormat.getSubdomainCallingFormat());
        
    }

    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey, boolean isSecure, 
                             String server, CallingFormat format) {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server, 
             isSecure ? Utils.SECURE_PORT : Utils.INSECURE_PORT, 
             format);
    }

    /**
     * Create a new interface to interact with S3 with the given credential and connection
     * parameters
     *
     * @param awsAccessKeyId Your user key into AWS
     * @param awsSecretAccessKey The secret string used to generate signatures for authentication.
     * @param isSecure use SSL encryption
     * @param server Which host to connect to.  Usually, this will be s3.amazonaws.com
     * @param port Which port to use.
     * @param callingFormat Type of request Regular/Vanity or Pure Vanity domain
     */
    public AWSAuthConnection(String awsAccessKeyId, String awsSecretAccessKey, boolean isSecure,
                             String server, int port, CallingFormat format)
    {
        this.awsAccessKeyId = awsAccessKeyId;
        this.awsSecretAccessKey = awsSecretAccessKey;
        this.isSecure = isSecure;
        this.server = server;
        this.port = port;
        this.callingFormat = format;
    }
    
    /**
     * Creates a new bucket.
     * @param bucket The name of the bucket to create.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     * @param metadata A Map of String to List of Strings representing the s3
     * metadata for this bucket (can be null).
     * @deprecated use version that specifies location
     */
    public Response createBucket(String bucket, Map headers)
        throws MalformedURLException, IOException
    {
        return createBucket(bucket, null, headers);
    }

    /**
     * Creates a new bucket.
     * @param bucket The name of the bucket to create.
     * @param location Desired location ("EU") (or null for default).
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     * @param metadata A Map of String to List of Strings representing the s3
     * metadata for this bucket (can be null).
     * @throws IllegalArgumentException on invalid location
     */
    public Response createBucket(String bucket, String location, Map headers)
        throws MalformedURLException, IOException
    {
        String body;
        if (location == null) {
            body = null;
        } else if (LOCATION_EU.equals(location)) {
            if (!callingFormat.supportsLocatedBuckets())
                throw new IllegalArgumentException("Creating location-constrained bucket with unsupported calling-format");
            body = "<CreateBucketConstraint><LocationConstraint>" + location + "</LocationConstraint></CreateBucketConstraint>";
        } else
            throw new IllegalArgumentException("Invalid Location: "+location);

        // validate bucket name
        if (!Utils.validateBucketName(bucket, callingFormat, location != null))
            throw new IllegalArgumentException("Invalid Bucket Name: "+bucket);

        HttpURLConnection request = makeRequest("PUT", bucket, "", null, headers);
        if (body != null)
        {
            request.setDoOutput(true);
            request.getOutputStream().write(body.getBytes("UTF-8"));
        }
        return new Response(request);
    }

    /**
     * Check if the specified bucket exists (via a HEAD request)
     * @param bucket The name of the bucket to check
     * @return true if HEAD access returned success
     */
    public boolean checkBucketExists(String bucket) throws MalformedURLException, IOException
    {
        HttpURLConnection response  = makeRequest("HEAD", bucket, "", null, null);
        int httpCode = response.getResponseCode();
        return httpCode >= 200 && httpCode < 300;
    }

    /**
     * Lists the contents of a bucket.
     * @param bucket The name of the bucket to create.
     * @param prefix All returned keys will start with this string (can be null).
     * @param marker All returned keys will be lexographically greater than
     * this string (can be null).
     * @param maxKeys The maximum number of keys to return (can be null).
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public ListBucketResponse listBucket(String bucket, String prefix, String marker,
                                         Integer maxKeys, Map headers)
        throws MalformedURLException, IOException
    {
        return listBucket(bucket, prefix, marker, maxKeys, null, headers);
    }

    /**
     * Lists the contents of a bucket.
     * @param bucket The name of the bucket to list.
     * @param prefix All returned keys will start with this string (can be null).
     * @param marker All returned keys will be lexographically greater than
     * this string (can be null).
     * @param maxKeys The maximum number of keys to return (can be null).
     * @param delimiter Keys that contain a string between the prefix and the first 
     * occurrence of the delimiter will be rolled up into a single element.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public ListBucketResponse listBucket(String bucket, String prefix, String marker,
                                         Integer maxKeys, String delimiter, Map headers)
        throws MalformedURLException, IOException
    {

        Map pathArgs = Utils.paramsForListOptions(prefix, marker, maxKeys, delimiter);
        return new ListBucketResponse(makeRequest("GET", bucket, "", pathArgs, headers));
    }

    /**
     * Deletes a bucket.
     * @param bucket The name of the bucket to delete.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response deleteBucket(String bucket, Map headers)
        throws MalformedURLException, IOException
    {
        return new Response(makeRequest("DELETE", bucket, "", null, headers));
    }

    /**
     * Writes an object to S3.
     * @param bucket The name of the bucket to which the object will be added.
     * @param key The name of the key to use.
     * @param object An S3Object containing the data to write.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response put(String bucket, String key, S3Object object, Map headers)
        throws MalformedURLException, IOException
    {
        HttpURLConnection request =
            makeRequest("PUT", bucket, Utils.urlencode(key), null, headers, object);

        request.setDoOutput(true);
        request.getOutputStream().write(object.data == null ? new byte[] {} : object.data);

        return new Response(request);
    }

    /**
     * Creates a copy of an existing S3 Object.  In this signature, we will copy the
     * existing metadata.  The default access control policy is private; if you want
     * to override it, please use x-amz-acl in the headers.
     * @param sourceBucket The name of the bucket where the source object lives.
     * @param sourceKey The name of the key to copy.
     * @param destinationBucket The name of the bucket to which the object will be added.
     * @param destinationKey The name of the key to use.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).  You may wish to set the x-amz-acl header appropriately.
     */
    public Response copy( String sourceBucket, String sourceKey, String destinationBucket, String destinationKey, Map headers )
    throws MalformedURLException, IOException
    {
        S3Object object = new S3Object(new byte[] {}, new HashMap());
        headers = headers == null ? new HashMap() : new HashMap(headers);
        headers.put("x-amz-copy-source", Arrays.asList( new String[] { sourceBucket + "/" + sourceKey } ) );
        headers.put("x-amz-metadata-directive", Arrays.asList( new String[] { "COPY" } ) );
        return verifyCopy( put( destinationBucket, destinationKey, object, headers ) );
    }

    /**
     * Creates a copy of an existing S3 Object.  In this signature, we will replace the
     * existing metadata.  The default access control policy is private; if you want
     * to override it, please use x-amz-acl in the headers.
     * @param sourceBucket The name of the bucket where the source object lives.
     * @param sourceKey The name of the key to copy.
     * @param destinationBucket The name of the bucket to which the object will be added.
     * @param destinationKey The name of the key to use.
     * @param metadata A Map of String to List of Strings representing the S3 metadata
     * for the new object.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).  You may wish to set the x-amz-acl header appropriately.
     */
    public Response copy( String sourceBucket, String sourceKey, String destinationBucket, String destinationKey, Map metadata, Map headers )
    throws MalformedURLException, IOException
    {
        S3Object object = new S3Object(new byte[] {}, metadata);
        headers = headers == null ? new HashMap() : new HashMap(headers);
        headers.put("x-amz-copy-source", Arrays.asList( new String[] { sourceBucket + "/" + sourceKey } ) );
        headers.put("x-amz-metadata-directive", Arrays.asList( new String[] { "REPLACE" } ) );
        return verifyCopy( put( destinationBucket, destinationKey, object, headers ) );
    }

    /**
     * Copy sometimes returns a successful response and starts to send whitespace
     * characters to us.  This method processes those whitespace characters and
     * will throw an exception if the response is either unknown or an error.
     * @param response Response object from the PUT request.
     * @return The response with the input stream drained.
     * @throws IOException If anything goes wrong.
     */
    private Response verifyCopy( Response response ) throws IOException {
        if (response.connection.getResponseCode() < 400) {
            byte[] body = GetResponse.slurpInputStream(response.connection.getInputStream());
            String message = new String( body );
            if ( message.indexOf( "<Error" ) != -1 ) {
                throw new IOException( message.substring( message.indexOf( "<Error" ) ) );
            } else if ( message.indexOf( "</CopyObjectResult>" ) != -1 ) {
                // It worked!
            } else {
                throw new IOException( "Unexpected response: " + message );
            }
        }
        return response;
    }

    /**
     * Reads an object from S3.
     * @param bucket The name of the bucket where the object lives.
     * @param key The name of the key to use.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public GetResponse get(String bucket, String key, Map headers)
        throws MalformedURLException, IOException
    {
        return new GetResponse(makeRequest("GET", bucket, Utils.urlencode(key), null, headers));
    }

    /**
     * Deletes an object from S3.
     * @param bucket The name of the bucket where the object lives.
     * @param key The name of the key to use.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response delete(String bucket, String key, Map headers)
        throws MalformedURLException, IOException
    {
        return new Response(makeRequest("DELETE", bucket, Utils.urlencode(key), null, headers));
    }

    /**
     * Get the requestPayment xml document for a given bucket
     * @param bucket The name of the bucket
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public GetResponse getBucketRequestPayment(String bucket, Map headers)
        throws MalformedURLException, IOException
    {
        Map pathArgs = new HashMap();
        pathArgs.put("requestPayment", null);
        return new GetResponse(makeRequest("GET", bucket, "", pathArgs, headers));
    }

    /**
     * Write a new requestPayment xml document for a given bucket
     * @param loggingXMLDoc The xml representation of the requestPayment configuration as a String
     * @param bucket The name of the bucket
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response putBucketRequestPayment(String bucket, String requestPaymentXMLDoc, Map headers)
        throws MalformedURLException, IOException
    {
        Map pathArgs = new HashMap();
        pathArgs.put("requestPayment", null);
        S3Object object = new S3Object(requestPaymentXMLDoc.getBytes(), null);
        HttpURLConnection request = makeRequest("PUT", bucket, "", pathArgs, headers, object);

        request.setDoOutput(true);
        request.getOutputStream().write(object.data == null ? new byte[] {} : object.data);

        return new Response(request);
    }
    
    /**
     * Get the logging xml document for a given bucket
     * @param bucket The name of the bucket
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public GetResponse getBucketLogging(String bucket, Map headers)
        throws MalformedURLException, IOException
    {
        Map pathArgs = new HashMap();
        pathArgs.put("logging", null);
        return new GetResponse(makeRequest("GET", bucket, "", pathArgs, headers));
    }

    /**
     * Write a new logging xml document for a given bucket
     * @param loggingXMLDoc The xml representation of the logging configuration as a String
     * @param bucket The name of the bucket
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response putBucketLogging(String bucket, String loggingXMLDoc, Map headers)
        throws MalformedURLException, IOException
    {
        Map pathArgs = new HashMap();
        pathArgs.put("logging", null);
        S3Object object = new S3Object(loggingXMLDoc.getBytes(), null);
        HttpURLConnection request = makeRequest("PUT", bucket, "", pathArgs, headers, object);

        request.setDoOutput(true);
        request.getOutputStream().write(object.data == null ? new byte[] {} : object.data);

        return new Response(request);
    }

    /**
     * Get the ACL for a given bucket
     * @param bucket The name of the bucket where the object lives.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public GetResponse getBucketACL(String bucket, Map headers)
        throws MalformedURLException, IOException
    {
        return getACL(bucket, "", headers);
    }

    /**
     * Get the ACL for a given object (or bucket, if key is null).
     * @param bucket The name of the bucket where the object lives.
     * @param key The name of the key to use.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public GetResponse getACL(String bucket, String key, Map headers)
        throws MalformedURLException, IOException
    {
        if (key == null) key = "";
        
        Map pathArgs = new HashMap();
        pathArgs.put("acl", null);
        
        return new GetResponse(
                makeRequest("GET", bucket, Utils.urlencode(key), pathArgs, headers)
            );
    }

    /**
     * Write a new ACL for a given bucket
     * @param aclXMLDoc The xml representation of the ACL as a String
     * @param bucket The name of the bucket where the object lives.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response putBucketACL(String bucket, String aclXMLDoc, Map headers)
        throws MalformedURLException, IOException
    {
        return putACL(bucket, "", aclXMLDoc, headers);
    }

    /**
     * Write a new ACL for a given object
     * @param aclXMLDoc The xml representation of the ACL as a String
     * @param bucket The name of the bucket where the object lives.
     * @param key The name of the key to use.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public Response putACL(String bucket, String key, String aclXMLDoc, Map headers)
        throws MalformedURLException, IOException
    {
        S3Object object = new S3Object(aclXMLDoc.getBytes(), null);
        
        Map pathArgs = new HashMap();
        pathArgs.put("acl", null);

        HttpURLConnection request =
            makeRequest("PUT", bucket, Utils.urlencode(key), pathArgs, headers, object);

        request.setDoOutput(true);
        request.getOutputStream().write(object.data == null ? new byte[] {} : object.data);

        return new Response(request);
    }

    public LocationResponse getBucketLocation(String bucket) 
        throws MalformedURLException, IOException 
    {
        Map pathArgs = new HashMap();
        pathArgs.put("location", null);
        return new LocationResponse(makeRequest("GET", bucket, "", pathArgs, null));
    }
        
    
    /**
     * List all the buckets created by this account.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    public ListAllMyBucketsResponse listAllMyBuckets(Map headers)
        throws MalformedURLException, IOException
    {
        return new ListAllMyBucketsResponse(makeRequest("GET", "", "", null, headers));
    }
    

    
    /**
     * Make a new HttpURLConnection without passing an S3Object parameter. 
     * Use this method for key operations that do require arguments
     * @param method The method to invoke
     * @param bucketName the bucket this request is for
     * @param key the key this request is for
     * @param pathArgs the 
     * @param headers
     * @return
     * @throws MalformedURLException
     * @throws IOException
     */
    private HttpURLConnection makeRequest(String method, String bucketName, String key, Map pathArgs, Map headers)
        throws MalformedURLException, IOException
    {
        return makeRequest(method, bucketName, key, pathArgs, headers, null);
    }
   
  

    /**
     * Make a new HttpURLConnection.
     * @param method The HTTP method to use (GET, PUT, DELETE)
     * @param bucketName The bucket name this request affects
     * @param key The key this request is for
     * @param pathArgs parameters if any to be sent along this request
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     * @param object The S3Object that is to be written (can be null).
     */
    private HttpURLConnection makeRequest(String method, String bucket, String key, Map pathArgs, Map headers,
                                          S3Object object)
        throws MalformedURLException, IOException
    {
        CallingFormat callingFormat = Utils.getCallingFormatForBucket( this.callingFormat, bucket );
        if ( isSecure && callingFormat != CallingFormat.getPathCallingFormat() && bucket.indexOf( "." ) != -1 ) {
            System.err.println( "You are making an SSL connection, however, the bucket contains periods and the wildcard certificate will not match by default.  Please consider using HTTP." );
        }

        // build the domain based on the calling format
        URL url = callingFormat.getURL(isSecure, server, this.port, bucket, key, pathArgs);
        
        HttpURLConnection connection = (HttpURLConnection)url.openConnection();
        connection.setRequestMethod(method);

        // subdomain-style urls may encounter http redirects.  
        // Ensure that redirects are supported.
        if (!connection.getInstanceFollowRedirects()
            && callingFormat.supportsLocatedBuckets())
            throw new RuntimeException("HTTP redirect support required.");

        addHeaders(connection, headers);
        if (object != null) addMetadataHeaders(connection, object.metadata);
        addAuthHeader(connection, method, bucket, key, pathArgs);

        return connection;
    }

    /**
     * Add the given headers to the HttpURLConnection.
     * @param connection The HttpURLConnection to which the headers will be added.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     */
    private void addHeaders(HttpURLConnection connection, Map headers) {
        addHeaders(connection, headers, "");
    }

    /**
     * Add the given metadata fields to the HttpURLConnection.
     * @param connection The HttpURLConnection to which the headers will be added.
     * @param metadata A Map of String to List of Strings representing the s3
     * metadata for this resource.
     */
    private void addMetadataHeaders(HttpURLConnection connection, Map metadata) {
        addHeaders(connection, metadata, Utils.METADATA_PREFIX);
    }

    /**
     * Add the given headers to the HttpURLConnection with a prefix before the keys.
     * @param connection The HttpURLConnection to which the headers will be added.
     * @param headers A Map of String to List of Strings representing the http
     * headers to pass (can be null).
     * @param prefix The string to prepend to each key before adding it to the connection.
     */
    private void addHeaders(HttpURLConnection connection, Map headers, String prefix) {
        if (headers != null) {
            for (Iterator i = headers.keySet().iterator(); i.hasNext(); ) {
                String key = (String)i.next();
                for (Iterator j = ((List)headers.get(key)).iterator(); j.hasNext(); ) {
                    String value = (String)j.next();
                    connection.addRequestProperty(prefix + key, value);
                }
            }
        }
    }

    /**
     * Add the appropriate Authorization header to the HttpURLConnection.
     * @param connection The HttpURLConnection to which the header will be added.
     * @param method The HTTP method to use (GET, PUT, DELETE)
     * @param bucket the bucket name this request is for
     * @param key the key this request is for
     * @param pathArgs path arguments which are part of this request
     */
    private void addAuthHeader(HttpURLConnection connection, String method, String bucket, String key, Map pathArgs) {
        if (connection.getRequestProperty("Date") == null) {
            connection.setRequestProperty("Date", httpDate());
        }
        if (connection.getRequestProperty("Content-Type") == null) {
            connection.setRequestProperty("Content-Type", "");
        }

        String canonicalString =
            Utils.makeCanonicalString(method, bucket, key, pathArgs, connection.getRequestProperties());
        String encodedCanonical = Utils.encode(this.awsSecretAccessKey, canonicalString, false);
        connection.setRequestProperty("Authorization",
                                      "AWS " + this.awsAccessKeyId + ":" + encodedCanonical);
    }


    /**
     * Generate an rfc822 date for use in the Date HTTP header.
     */
    public static String httpDate() {
        final String DateFormat = "EEE, dd MMM yyyy HH:mm:ss ";
        SimpleDateFormat format = new SimpleDateFormat( DateFormat, Locale.US );
        format.setTimeZone( TimeZone.getTimeZone( "GMT" ) );
        return format.format( new Date() ) + "GMT";
    }
}
