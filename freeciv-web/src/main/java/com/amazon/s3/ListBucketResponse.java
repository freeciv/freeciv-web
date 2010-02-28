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
//  this software code. (c) 2006 Amazon Digital Services, Inc. or its
//  affiliates.

package com.amazon.s3;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.SimpleTimeZone;

import org.xml.sax.Attributes;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.InputSource;
import org.xml.sax.XMLReader;
import org.xml.sax.SAXException;


/**
 * Returned by AWSAuthConnection.listBucket()
 */
public class ListBucketResponse extends Response {

    /**
     * The name of the bucket being listed.  Null if request fails.
     */
    public String name = null;

    /**
     * The prefix echoed back from the request.  Null if request fails.
     */
    public String prefix = null;

    /**
     * The marker echoed back from the request.  Null if request fails.
     */
    public String marker = null;

    /**
     * The delimiter echoed back from the request.  Null if not specified in
     * the request, or if it fails.
     */
    public String delimiter = null;

    /**
     * The maxKeys echoed back from the request if specified.  0 if request fails.
     */
    public int maxKeys = 0;

    /**
     * Indicates if there are more results to the list.  True if the current
     * list results have been truncated.  false if request fails.
     */
    public boolean isTruncated = false;

    /**
     * Indicates what to use as a marker for subsequent list requests in the event
     * that the results are truncated.  Present only when a delimiter is specified.  
     * Null if request fails.
     */
    public String nextMarker = null;

    /**
     * A List of ListEntry objects representing the objects in the given bucket.  
     * Null if the request fails.
     */
    public List entries = null;

    /**
     * A List of CommonPrefixEntry objects representing the common prefixes of the
     * keys that matched up to the delimiter.  Null if the request fails.
     */
    public List commonPrefixEntries = null;

    public ListBucketResponse(HttpURLConnection connection) throws IOException {
        super(connection);
        if (connection.getResponseCode() < 400) {
            try {
                XMLReader xr = Utils.createXMLReader();
                ListBucketHandler handler = new ListBucketHandler();
                xr.setContentHandler(handler);
                xr.setErrorHandler(handler);

                xr.parse(new InputSource(connection.getInputStream()));

                this.name = handler.getName();
                this.prefix = handler.getPrefix();
                this.marker = handler.getMarker();
                this.delimiter = handler.getDelimiter();
                this.maxKeys = handler.getMaxKeys();
                this.isTruncated = handler.getIsTruncated();
                this.nextMarker = handler.getNextMarker();
                this.entries = handler.getKeyEntries();
                this.commonPrefixEntries = handler.getCommonPrefixEntries();

            } catch (SAXException e) {
                throw new RuntimeException("Unexpected error parsing ListBucket xml", e);
            }
        }
    }

    class ListBucketHandler extends DefaultHandler {

        private String name = null;
        private String prefix = null;
        private String marker = null;
        private String delimiter = null;
        private int maxKeys = 0;
        private boolean isTruncated = false;
        private String nextMarker = null;
        private boolean isEchoedPrefix = false;
        private List keyEntries = null;
        private ListEntry keyEntry = null;
        private List commonPrefixEntries = null;
        private CommonPrefixEntry commonPrefixEntry = null;
        private StringBuffer currText = null;
        private SimpleDateFormat iso8601Parser = null;

        public ListBucketHandler() {
            super();
            keyEntries = new ArrayList();
            commonPrefixEntries = new ArrayList();
            this.iso8601Parser = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'");
            this.iso8601Parser.setTimeZone(new SimpleTimeZone(0, "GMT"));
            this.currText = new StringBuffer();
        }

        public void startDocument() {
            this.isEchoedPrefix = true;
        }

        public void endDocument() {
            // ignore
        }

        public void startElement(String uri, String name, String qName, Attributes attrs) {
            if (name.equals("Contents")) {
                this.keyEntry = new ListEntry();
            } else if (name.equals("Owner")) {
                this.keyEntry.owner = new Owner();
            } else if (name.equals("CommonPrefixes")){
                this.commonPrefixEntry = new CommonPrefixEntry();
            }
        }

        public void endElement(String uri, String name, String qName) {
            if (name.equals("Name")) {
                this.name = this.currText.toString();
            } 
            // this prefix is the one we echo back from the request
            else if (name.equals("Prefix") && this.isEchoedPrefix) {
                this.prefix = this.currText.toString();
                this.isEchoedPrefix = false;
            } else if (name.equals("Marker")) {
                this.marker = this.currText.toString();
            } else if (name.equals("MaxKeys")) {
                this.maxKeys = Integer.parseInt(this.currText.toString());
            } else if (name.equals("Delimiter")) {
                this.delimiter = this.currText.toString();
            } else if (name.equals("IsTruncated")) {
                this.isTruncated = Boolean.valueOf(this.currText.toString());
            } else if (name.equals("NextMarker")) {
                this.nextMarker = this.currText.toString();
            } else if (name.equals("Contents")) {
                this.keyEntries.add(this.keyEntry);
            } else if (name.equals("Key")) {
                this.keyEntry.key = this.currText.toString();
            } else if (name.equals("LastModified")) {
                try {
                    this.keyEntry.lastModified = this.iso8601Parser.parse(this.currText.toString());
                } catch (ParseException e) {
                    throw new RuntimeException("Unexpected date format in list bucket output", e);
                }
            } else if (name.equals("ETag")) {
                this.keyEntry.eTag = this.currText.toString();
            } else if (name.equals("Size")) {
                this.keyEntry.size = Long.parseLong(this.currText.toString());
            } else if (name.equals("StorageClass")) {
                this.keyEntry.storageClass = this.currText.toString();
            } else if (name.equals("ID")) {
                this.keyEntry.owner.id = this.currText.toString();
            } else if (name.equals("DisplayName")) {
                this.keyEntry.owner.displayName = this.currText.toString();
            } else if (name.equals("CommonPrefixes")) {
                this.commonPrefixEntries.add(this.commonPrefixEntry);
            }
            // this is the common prefix for keys that match up to the delimiter 
            else if (name.equals("Prefix")) {
                this.commonPrefixEntry.prefix = this.currText.toString();
            }
            if(this.currText.length() != 0)
                this.currText = new StringBuffer();
        }

        public void characters(char ch[], int start, int length) {
            this.currText.append(ch, start, length);
        }

        public String getName() {
            return this.name;
        }

        public String getPrefix() {
            return this.prefix;
        }

        public String getMarker() {
            return this.marker;
        }

        public String getDelimiter() {
            return this.delimiter;
        }

        public int getMaxKeys(){
            return this.maxKeys;
        }

        public boolean getIsTruncated() {
            return this.isTruncated;
        }

        public String getNextMarker() {
            return this.nextMarker;
        }

        public List getKeyEntries() {
            return this.keyEntries;
        }

        public List getCommonPrefixEntries() {
            return this.commonPrefixEntries;
        }
    }
}
