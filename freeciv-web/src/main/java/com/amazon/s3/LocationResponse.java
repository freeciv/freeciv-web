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

import java.io.IOException;
import java.net.HttpURLConnection;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;

/**
 * A Response object returned from AWSAuthConnection.getBucketLocation().
 * Parses the response XML and exposes the location constraint
 * via the geteLocation() method.
 */
public class LocationResponse extends Response {
    String location;
    
    /**
     * Parse the response to a ?location query.
     */
    public LocationResponse(HttpURLConnection connection) throws IOException {
        super(connection);
        if (connection.getResponseCode() < 400) {
            try {
                XMLReader xr = Utils.createXMLReader();;
                LocationResponseHandler handler = new LocationResponseHandler();
                xr.setContentHandler(handler);
                xr.setErrorHandler(handler);

                xr.parse(new InputSource(connection.getInputStream()));
                this.location = handler.location;
            } catch (SAXException e) {
                throw new RuntimeException("Unexpected error parsing ListAllMyBuckets xml", e);
            }
        } else {
            this.location = "<error>";
        }
    }

    /**
     * Report the location-constraint for a bucket.
     * A value of null indicates an error; 
     * the empty string indicates no constraint;
     * and any other value is an actual location constraint value.
     */
    public String getLocation() {
        return location;
    }

    /**
     * Helper class to parse LocationConstraint response XML
     */
    static class LocationResponseHandler extends DefaultHandler {
        String location = null;
        private StringBuffer currText = null;
        
        public void startDocument() {
        }

        public void startElement(String uri, String name, String qName, Attributes attrs) {
            if (name.equals("LocationConstraint")) {
                this.currText = new StringBuffer();
            }
        }

        public void endElement(String uri, String name, String qName) {
            if (name.equals("LocationConstraint")) {
                location = this.currText.toString();
                this.currText = null;
            }
        }
        
        public void characters(char ch[], int start, int length) {
            if (currText != null)
                this.currText.append(ch, start, length);
        }
    }
}
