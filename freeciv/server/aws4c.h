
/*
 *
 * Copyright(c) 2009,  Vlad Korolev,  <vlad[@]v-lad.org >
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://www.gnu.org/licenses/lgpl-3.0.txt
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 */


/// IOBuf Node
typedef struct _IOBufNode
{
  char * buf;
  struct _IOBufNode * next;
} IOBufNode;

/// IOBuf structure
typedef struct IOBuf 
{
  IOBufNode * first;
  IOBufNode * current;
  char   * pos;

  char * result;
  char * lastMod;
  char * eTag;
  int contentLen;
  int len;
  int code;

} IOBuf;



void aws_init ();
void aws_set_id ( char * const str );    
void aws_set_key ( char * const str );
void aws_set_keyid ( char * const str );
int aws_read_config ( char * const ID );
void aws_set_debug (int d);


void s3_set_bucket ( char * const str );
int s3_get ( IOBuf * b, char * const file );
int s3_put ( IOBuf * b, char * const file );
void s3_set_host ( char * const str );
void s3_set_mime ( char * const str );
void s3_set_acl ( char * const str );


int sqs_create_queue ( IOBuf *b, char * const name );
int sqs_list_queues ( IOBuf *b, char * const prefix );
int sqs_get_queueattributes ( IOBuf *b, char * url, int *TimeOut, int *nMesg );
int sqs_set_queuevisibilitytimeout ( IOBuf *b, char * url, int sec );
int sqs_get_message ( IOBuf * b, char * const url, char * id  );
int sqs_send_message ( IOBuf *b, char * const url, char * const msg );
int sqs_delete_message ( IOBuf * bf, char * const url, char * receipt );

IOBuf * aws_iobuf_new ();
void   aws_iobuf_append ( IOBuf *B, char * d, int len );
int    aws_iobuf_getline   ( IOBuf * B, char * Line, int size );
void   aws_iobuf_free ( IOBuf * bf );

