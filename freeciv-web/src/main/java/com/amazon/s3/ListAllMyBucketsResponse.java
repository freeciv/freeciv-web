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
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;

/**
 * Returned by AWSAuthConnection.listAllMyBuckets().
 */
public class ListAllMyBucketsResponse extends Response {
    /**
     * A list of Bucket objects, one for each of this account's buckets.  Will be null if
     * the request fails.
     */
    public List entries;

    public ListAllMyBucketsResponse(HttpURLConnection connection) throws IOException {
        super(connection);
        if (connection.getResponseCode() < 400) {
            try {
                XMLReader xr = Utils.createXMLReader();;
                ListAllMyBucketsHandler handler = new ListAllMyBucketsHandler();
                xr.setContentHandler(handler);
                xr.setErrorHandler(handler);

                xr.parse(new InputSource(connection.getInputStream()));
                this.entries = handler.getEntries();
            } catch (SAXException e) {
                throw new RuntimeException("Unexpected error parsing ListAllMyBuckets xml", e);
            }
        }
    }

    static class ListAllMyBucketsHandler extends DefaultHandler {

        private List entries = null;
        private Bucket currBucket = null;
        private StringBuffer currText = null;
        private SimpleDateFormat iso8601Parser = null;

        public ListAllMyBucketsHandler() {
            super();
            entries = new ArrayList();
            this.iso8601Parser = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'");
            this.iso8601Parser.setTimeZone(new SimpleTimeZone(0, "GMT"));
            this.currText = new StringBuffer();
        }

        public void startDocument() {
            // ignore
        }

        public void endDocument() {
            // ignore
        }

        public void startElement(String uri, String name, String qName, Attributes attrs) {
            if (name.equals("Bucket")) {
                this.currBucket = new Bucket();
            }
        }

        public void endElement(String uri, String name, String qName) {
            if (name.equals("Bucket")) {
                this.entries.add(this.currBucket);
            } else if (name.equals("Name")) {
                this.currBucket.name = this.currText.toString();
            } else if (name.equals("CreationDate")) {
                try {
                    this.currBucket.creationDate = this.iso8601Parser.parse(this.currText.toString());
                } catch (ParseException e) {
                    throw new RuntimeException("Unexpected date format in list bucket output", e);
                }
            }
            this.currText = new StringBuffer();
        }

        public void characters(char ch[], int start, int length) {
            this.currText.append(ch, start, length);
        }

        public List getEntries() {
            return this.entries;
        }
    }
}

