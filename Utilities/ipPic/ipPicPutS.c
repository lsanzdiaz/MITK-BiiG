
/*
 * $RCSfile$
 *--------------------------------------------------------------------
 * DESCRIPTION
 *   writes a PicFile to disk
 *
 * $Log$
 * Revision 1.1  1997/09/06 19:09:59  andre
 * Initial revision
 *
 * Revision 0.0  1993/04/02  16:18:39  andre
 * Initial revision
 *
 *
 *--------------------------------------------------------------------
 *  COPYRIGHT (c) 1993 by DKFZ (Dept. MBI) Heidelberg, FRG
 */
#ifndef lint
  static char *what = { "@(#)ipPicPutSlice\t\tDKFZ (Dept. MBI)\t$Date$" };
#endif

#include "ipPic.h"

void ipPicPutSlice( char *outfile_name, ipPicDescriptor *pic, ipUInt4_t slice )
{
  ipPicDescriptor *pic_in;

  ipUInt4_t tags_len;

  FILE *outfile;

  pic_in = ipPicGetHeader( outfile_name,
                           NULL );

  if( pic_in == NULL )
    if( slice == 1 )
      {
        pic->n[pic->dim] = 1;
        pic->dim += 1;

        ipPicPut( outfile_name, pic );

        pic->dim -= 1;
        pic->n[pic->dim] = 0;

        return;
      }
    else
      return;

  outfile = fopen( outfile_name, "r+b" );

  if( outfile == NULL )
    {
      /*ipPrintErr( "ipPicPut: sorry, error opening outfile\n" );*/
      /*return();*/
    }

  if( pic->dim != pic_in->dim - 1 )
    {
      fclose( outfile );
      return;
    }

  if( slice > pic_in->n[pic_in->dim-1] )
    pic_in->n[pic_in->dim-1] += 1;

  /* write oufile */
  /*fseek( outfile, 0, SEEK_SET );*/
  rewind( outfile );
  fwrite( ipPicVERSION, 1, sizeof(ipPicTag_t), outfile );

  ipFReadLE( &tags_len, sizeof(ipUInt4_t), 1, outfile );
  tags_len = tags_len - 3 * sizeof(ipUInt4_t)
                      - pic_in->dim * sizeof(ipUInt4_t);

  ipFWriteLE( &(pic_in->type), sizeof(ipUInt4_t), 1, outfile );
  ipFWriteLE( &(pic_in->bpe), sizeof(ipUInt4_t), 1, outfile );
  ipFWriteLE( &(pic_in->dim), sizeof(ipUInt4_t), 1, outfile );

  ipFWriteLE( pic_in->n, sizeof(ipUInt4_t), pic_in->dim, outfile );

  fseek( outfile, tags_len + _ipPicSize(pic) * (slice - 1), SEEK_CUR );

  ipFWriteLE( pic->data, pic->bpe / 8, _ipPicElements(pic), outfile );

  /*fseek( outfile, 0, SEEK_END );*/

  fclose( outfile );

  ipPicFree(pic_in);

  /*return();*/
}
