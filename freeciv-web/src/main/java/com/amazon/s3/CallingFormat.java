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
import java.util.Map;

public abstract class CallingFormat {
    
    protected static CallingFormat pathCallingFormat = new PathCallingFormat();
    protected static CallingFormat subdomainCallingFormat = new SubdomainCallingFormat();
    protected static CallingFormat vanityCallingFormat = new VanityCallingFormat();
    
    public abstract boolean supportsLocatedBuckets();
    public abstract String getEndpoint(String server, int port, String bucket);
    public abstract String getPathBase(String bucket, String key);
    public abstract URL getURL (boolean isSecure, String server, int port, String bucket, String key, Map pathArgs)
        throws MalformedURLException;
    
    public static CallingFormat getPathCallingFormat() {
        return pathCallingFormat;
    }
    
    public static CallingFormat getSubdomainCallingFormat() {
        return subdomainCallingFormat;
    }
    
    public static CallingFormat getVanityCallingFormat() {
        return vanityCallingFormat;
    }
    
    static private class PathCallingFormat extends CallingFormat {
        public boolean supportsLocatedBuckets() { 
            return false; 
        }
        
        public String getPathBase(String bucket, String key) {
            return isBucketSpecified(bucket) ? "/" + bucket + "/" + key : "/";
        }
        
        public String getEndpoint(String server, int port, String bucket) {
            return server + ":" + port;
        }
        
        public URL getURL(boolean isSecure, String server, int port, String bucket, String key, Map pathArgs) 
        throws MalformedURLException {
           String pathBase = isBucketSpecified(bucket) ? "/" + bucket + "/" + key : "/";
           String pathArguments = Utils.convertPathArgsHashToString(pathArgs);
           return new URL(isSecure ? "https": "http", server, port, pathBase + pathArguments);
        }

        private boolean isBucketSpecified(String bucket) {
            if(bucket == null) return false;
            if(bucket.length() == 0) return false;
            return true;
        }
    }

    static private class SubdomainCallingFormat extends CallingFormat {
        public boolean supportsLocatedBuckets() { 
            return true; 
        }

        public String getServer(String server, String bucket) {
            return bucket + "." + server;
        }
        public String getEndpoint(String server, int port, String bucket) {
            return getServer(server, bucket) + ":" + port ;
        }
        public String getPathBase(String bucket, String key) {
            return "/" + key;
        }
        
        public URL getURL(boolean isSecure, String server, int port, String bucket, String key, Map pathArgs) 
        throws MalformedURLException {
            if (bucket == null || bucket.length() == 0)
            {
                //The bucket is null, this is listAllBuckets request
                String pathArguments = Utils.convertPathArgsHashToString(pathArgs);
                return new URL(isSecure ? "https": "http", server, port, "/" + pathArguments);
            } else {
                String serverToUse = getServer(server, bucket);
                String pathBase = getPathBase(bucket, key);
                String pathArguments = Utils.convertPathArgsHashToString(pathArgs);
                return new URL(isSecure ? "https": "http", serverToUse, port, pathBase + pathArguments);
            }
        }
    }

    static private class VanityCallingFormat extends SubdomainCallingFormat {
        public String getServer(String server, String bucket) {
            return bucket;
        }
    }
}
