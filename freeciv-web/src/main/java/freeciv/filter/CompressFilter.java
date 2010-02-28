/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


package freeciv.filter;


import javax.servlet.Filter;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.FilterChain;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpServletResponseWrapper;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.ByteArrayOutputStream;
import java.util.zip.GZIPOutputStream;
import java.util.zip.ZipOutputStream;
import java.util.zip.ZipEntry;
import java.util.Set;
import java.util.HashSet;
import java.util.StringTokenizer;

public class CompressFilter implements Filter
{

   private Set exclude = new HashSet();

   public void init(FilterConfig config) throws ServletException
   {
//      String reject = config.getInitParameter("reject");
      String reject = "application/pdf,application/x-zip-compressed,application/x-gzip-compressed,audio/x-mp3";
      StringTokenizer tk = new StringTokenizer(reject, ",");
      while (tk.hasMoreTokens())
      {
         exclude.add(tk.nextToken().trim());
      }
   }

   public void destroy()
   {
      exclude.clear();
   }

   public void doFilter(ServletRequest req,
                        ServletResponse resp,
                        FilterChain chain) throws ServletException, IOException
   {
      final HttpServletRequest request = (HttpServletRequest) req;
      final HttpServletResponse response = (HttpServletResponse) resp;

      // setup streams
      final OutputStream servletOut = response.getOutputStream();
      final ByteArrayOutputStream baos = new ByteArrayOutputStream();
      final ServletOutputStreamWrapper wrappedStream = new ServletOutputStreamWrapper(baos);

      //
      final String[] contentType = new String[1];

      // wrap the used stream within a servlet
      // stream that will be given to process the request
      HttpServletResponse wrappedReponse = new HttpServletResponseWrapper(response)
      {
         public ServletOutputStream getOutputStream() throws IOException
         {
            return wrappedStream;
         }
         public PrintWriter getWriter() throws IOException
         {
            return new PrintWriter(wrappedStream);
         }
         public void setContentType(String type)
         {
            contentType[0] = type;
            if (exclude.contains(type))
            {
               // that means we copy the stream directly
               // suitable for streams
               response.setContentType(contentType[0]);
               response.setBufferSize(8192);
               try
               {
                  servletOut.write(baos.toByteArray());
                  // todo free the baos to help the GC
               }
               catch (IOException e)
               {
                  e.printStackTrace();
               }
               wrappedStream.wrapped = servletOut;
            }
         }
      };

      // finally do the request
      chain.doFilter(request, wrappedReponse);

      if (wrappedStream.wrapped == servletOut)
      {
         // that means the resource has already be sent
         servletOut.close();
         return;
      }

      // todo
      // if the response has been commited
      // we suppose it has been by another mechanism
      // than the usual one, meaning it's an error
      // that's a hack - the wrapper should intercept :
      // - close() on the outputstream : do nothing, it will be compressed
      // - sendError() on the response : discard the buffer and do nothing else
      if (!response.isCommitted())
      {
         // flush
         baos.close();

         // get bytes
         byte[] bytes = baos.toByteArray();

         // get browser info
         String encodings = null;
         if (contentType[0] == null)
         {
            encodings = request.getHeader("accept-encoding");
         }

 	 // determines if the
         // browser support any kind of compression
         // if so modify the streams
         int action = 1;  //gzip is default.
         if (encodings != null)
         {
            if (encodings.indexOf("gzip") != -1)
            {
               action = 1;
            }
            else if (encodings.indexOf("compress") != -1)
            {
               action = 2;
            }
         }

         if (action != 0)
         {
            baos.reset();
            if (action == 1)
            {
               response.setHeader("Content-Encoding", "gzip");
               GZIPOutputStream gzip = new GZIPOutputStream(baos);
               gzip.write(bytes);
               gzip.close();
            }
            else
            {
               response.setHeader("Content-Encoding", "x-compress");
               ZipOutputStream zip = new ZipOutputStream(baos);
               zip.putNextEntry(new ZipEntry("some name"));
               zip.write(bytes);
               zip.closeEntry();
               zip.close();
	         
            }
            bytes = baos.toByteArray();
         }

         if (contentType[0] != null)
         {
            response.setContentType(contentType[0]);
         }
         response.setBufferSize(bytes.length);
         response.setContentLength(bytes.length);
         servletOut.write(bytes);
         servletOut.close();
      }
   }

   // wrap an outputstream within s servlet outputstream
   class ServletOutputStreamWrapper extends ServletOutputStream
   {

      private OutputStream wrapped;

      public ServletOutputStreamWrapper(OutputStream wrapped)
      {
         this.wrapped = wrapped;
      }

      public OutputStream getWrapped()
      {
         return wrapped;
      }

      public void write(int b) throws IOException
      {
         wrapped.write(b);
      }

      public void write(byte b[], int off, int len) throws IOException
      {
         wrapped.write(b, off, len);
      }

      public void write(byte b[]) throws IOException
      {
         wrapped.write(b);
      }

      public void flush() throws IOException
      {
         // do nothing
      }

      public void close() throws IOException
      {
         // do nothing
      }

   }

}
