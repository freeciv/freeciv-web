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

import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * This class mimics the behavior of AWSAuthConnection, except instead of actually performing
 * the operation, QueryStringAuthGenerator will return URLs with query string parameters that
 * can be used to do the same thing.  These parameters include an expiration date, so that
 * if you hand them off to someone else, they will only work for a limited amount of time.
 */
public class QueryStringAuthGenerator {

    private String awsAccessKeyId;
    private String awsSecretAccessKey;
    private boolean isSecure;
    private String server;
    private int port;
    private CallingFormat callingFormat;

    private Long expiresIn = null;
    private Long expires = null;

    // by default, expire in 1 minute.
    private static final Long DEFAULT_EXPIRES_IN = new Long(60 * 1000);

    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey) {
        this(awsAccessKeyId, awsSecretAccessKey, true);
    }

    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey, 
                                    boolean isSecure)
    {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, Utils.DEFAULT_HOST);
    }

    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey, 
                                    boolean isSecure, String server)
    {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server,
             isSecure ? Utils.SECURE_PORT : Utils.INSECURE_PORT);
    }

    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey, 
                                    boolean isSecure, String server, int port)
    {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server,
             port, CallingFormat.getSubdomainCallingFormat());
    }
   
    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey, 
                                    boolean isSecure, String server, CallingFormat callingFormat)
    {
        this(awsAccessKeyId, awsSecretAccessKey, isSecure, server, 
             isSecure ? Utils.SECURE_PORT : Utils.INSECURE_PORT, 
             callingFormat);
    }

    public QueryStringAuthGenerator(String awsAccessKeyId, String awsSecretAccessKey,
                             boolean isSecure, String server, int port, CallingFormat callingFormat)
    {
        this.awsAccessKeyId = awsAccessKeyId;
        this.awsSecretAccessKey = awsSecretAccessKey;
        this.isSecure = isSecure;
        this.server = server;
        this.port = port;
        this.callingFormat = callingFormat;

        this.expiresIn = DEFAULT_EXPIRES_IN;
        this.expires = null;
    }
    

    public void setCallingFormat(CallingFormat format) {
        this.callingFormat = format;
    }

    public void setExpires(long millisSinceEpoch) {
        expires = new Long(millisSinceEpoch);
        expiresIn = null;
    }

    public void setExpiresIn(long millis) {
        expiresIn = new Long(millis);
        expires = null;
    }

    public String createBucket(String bucket, Map headers)
    {
        // validate bucket name
        if (!Utils.validateBucketName(bucket, callingFormat, false))
            throw new IllegalArgumentException("Invalid Bucket Name: "+bucket);

        Map pathArgs = new HashMap();
        return generateURL("PUT", bucket, "", pathArgs, headers);
    }

    public String listBucket(String bucket, String prefix, String marker,
                             Integer maxKeys, Map headers){
        return listBucket(bucket, prefix, marker, maxKeys, null, headers);
    }

    public String listBucket(String bucket, String prefix, String marker,
                             Integer maxKeys, String delimiter, Map headers)
    { 
        Map pathArgs = Utils.paramsForListOptions(prefix, marker, maxKeys, delimiter);
        return generateURL("GET", bucket, "", pathArgs, headers);
    }

    public String deleteBucket(String bucket, Map headers)
    {
        Map pathArgs = new HashMap();
        return generateURL("DELETE", bucket, "", pathArgs, headers);
    }

    public String put(String bucket, String key, S3Object object, Map headers) {
        Map metadata = null;
        Map pathArgs = new HashMap();
        if (object != null) {
            metadata = object.metadata;
        }
        

        return generateURL("PUT", bucket, Utils.urlencode(key), pathArgs, mergeMeta(headers, metadata));
    }

    public String get(String bucket, String key, Map headers)
    {
        Map pathArgs = new HashMap();
        return generateURL("GET", bucket, Utils.urlencode(key), pathArgs, headers);
    }

    public String delete(String bucket, String key, Map headers)
    {
        Map pathArgs = new HashMap();
        return generateURL("DELETE", bucket, Utils.urlencode(key), pathArgs, headers);
    }

    public String getBucketLogging(String bucket, Map headers) {
        Map pathArgs = new HashMap();
        pathArgs.put("logging", null);
        return generateURL("GET", bucket, "", pathArgs, headers);
    }

    public String putBucketLogging(String bucket, String loggingXMLDoc, Map headers) {
        Map pathArgs = new HashMap();
        pathArgs.put("logging", null);
        return generateURL("PUT", bucket, "", pathArgs, headers);
    }

    public String getBucketACL(String bucket, Map headers) {
        return getACL(bucket, "", headers);
    }

    public String getACL(String bucket, String key, Map headers)
    {
        Map pathArgs = new HashMap();
        pathArgs.put("acl", null);
        return generateURL("GET", bucket, Utils.urlencode(key), pathArgs, headers);
    }

    public String putBucketACL(String bucket, String aclXMLDoc, Map headers) {
        return putACL(bucket, "", aclXMLDoc, headers);
    }

    public String putACL(String bucket, String key, String aclXMLDoc, Map headers)
    {
        Map pathArgs = new HashMap();
        pathArgs.put("acl", null);
        return generateURL("PUT", bucket, Utils.urlencode(key), pathArgs, headers);
    }

    public String listAllMyBuckets(Map headers)
    {
        Map pathArgs = new HashMap();
        return generateURL("GET", "", "", pathArgs, headers);
    }

    public String makeBareURL(String bucket, String key) {
        StringBuffer buffer = new StringBuffer();
        if (this.isSecure) {
            buffer.append("https://");
        } else {
            buffer.append("http://");
        }
        buffer.append(this.server).append(":").append(this.port).append("/").append(bucket);
        buffer.append("/").append(Utils.urlencode(key));

        return buffer.toString();
    }

    private String generateURL(String method, String bucketName, String key, Map pathArgs, Map headers) 
    {
        long expires = 0L;
        if (this.expiresIn != null) {
            expires = System.currentTimeMillis() + this.expiresIn.longValue();
        } else if (this.expires != null) {
            expires = this.expires.longValue();
        } else {
            throw new RuntimeException("Illegal expires state");
        }

        // convert to seconds
        expires /= 1000;

        String canonicalString = Utils.makeCanonicalString(method, bucketName, key, pathArgs, headers, ""+expires);
        String encodedCanonical = Utils.encode(this.awsSecretAccessKey, canonicalString, true);

        pathArgs.put("Signature", encodedCanonical);
        pathArgs.put("Expires", Long.toString(expires));
        pathArgs.put("AWSAccessKeyId", this.awsAccessKeyId);
        
        CallingFormat callingFormat = Utils.getCallingFormatForBucket( this.callingFormat, bucketName );
        if ( isSecure && callingFormat != CallingFormat.getPathCallingFormat() && bucketName.indexOf( "." ) != -1 ) {
            System.err.println( "You are making an SSL connection, however, the bucket contains periods and the wildcard certificate will not match by default.  Please consider using HTTP." );
        }

        String returnString;
        try {
            URL url = callingFormat.getURL(isSecure, server, port, bucketName, key, pathArgs);
            returnString = url.toString();
        } catch (MalformedURLException e) {
            returnString = "Exception generating url " + e;
        }
        
        return returnString;
    }

    private Map mergeMeta(Map headers, Map metadata) {
        Map merged = new TreeMap();
        if (headers != null) {
            for (Iterator i = headers.keySet().iterator(); i.hasNext(); ) {
                String key = (String)i.next();
                merged.put(key, headers.get(key));
            }
        }
        if (metadata != null) {
            for (Iterator i = metadata.keySet().iterator(); i.hasNext(); ) {
                String key = (String)i.next();
                String metadataKey = Utils.METADATA_PREFIX + key;
                if (merged.containsKey(metadataKey)) {
                    ((List)merged.get(metadataKey)).addAll((List)metadata.get(key));
                } else {
                    merged.put(metadataKey, metadata.get(key));
                }
            }
        }
        return merged;
    }
}
