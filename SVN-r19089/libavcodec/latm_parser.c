/*
 * copyright (c) 2008 Paul Kendall <paul@kcbbs.gen.nz>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
  *
 * FFmpeg is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with FFmpeg; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
  */

 /**
  * @file latm_parser.c
  * LATM parser
  */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <sys/types.h>

 #include "parser.h"

 #define LATM_HEADER     0x56e000        // 0x2b7 (11 bits)
 #define LATM_MASK       0xFFE000        // top 11 bits
 #define LATM_SIZE_MASK  0x001FFF        // bottom 13 bits

 typedef struct LATMParseContext{
     ParseContext pc;
     int count;
 } LATMParseContext;

 /**
  * finds the end of the current frame in the bitstream.
  * @return the position of the first byte of the next frame, or -1
  */
 static int latm_find_frame_end(AVCodecParserContext *s1, const uint8_t *buf,
                                int buf_size) {
     LATMParseContext *s = s1->priv_data;
     ParseContext *pc = &s->pc;
     int pic_found, i;
     uint32_t state;

    int frame_count_in_buffer=0;
     state = pc->state;
    //av_log(NULL,AV_LOG_INFO,"s1->previous_frame_start_index = %d,s1->cur_frame_start_index=%d\n ",s1->previous_frame_start_index ,s1->cur_frame_start_index);
    if(s1->previous_frame_start_index != s1->cur_frame_start_index){
        for(i=0; i<buf_size; i++){
            state = (state<<8) | buf[i];
            if((state & LATM_MASK) == LATM_HEADER) {
                frame_count_in_buffer++;
            }
        }
        s1->previous_frame_start_index =  s1->cur_frame_start_index;
        s1->frame_count_inbuffer = frame_count_in_buffer;
        //av_log(NULL,AV_LOG_INFO,"************************ UPDATE FRAME_COUNT_INBUFFER [%d]\n ",s1->frame_count_inbuffer);
        //av_log(NULL,AV_LOG_INFO,"************************ UPDATE FRAME_COUNT_INBUFFER [%d]\n ",s1->frame_count_inbuffer);
        //av_log(NULL,AV_LOG_INFO,"************************ UPDATE FRAME_COUNT_INBUFFER [%d]\n ",s1->frame_count_inbuffer);
        //av_log(NULL,AV_LOG_INFO,"************************ UPDATE FRAME_COUNT_INBUFFER [%d]\n ",s1->frame_count_inbuffer);
    }

     pic_found = pc->frame_start_found;
     state = pc->state;

     i = 0;
     if(!pic_found){
         for(i=0; i<buf_size; i++){
             state = (state<<8) | buf[i];
             if((state & LATM_MASK) == LATM_HEADER) {
                 i++;
                 s->count = - i;
                 pic_found=1;
                 break;
             }
         }
     }
    //av_log(NULL,AV_LOG_INFO,"\n[latm_find_frame_end] s->count=%d \n",s->count);
     if(pic_found){
         /* EOF considered as end of frame */
         if (buf_size == 0)
             return 0;
         if((state & LATM_SIZE_MASK) - s->count <= buf_size) {
             pc->frame_start_found = 0;
             pc->state = -1;
             return (state & LATM_SIZE_MASK) - s->count;
         }
     }

     s->count += buf_size;
     pc->frame_start_found = pic_found;
     pc->state = state;
     return END_NOT_FOUND;
 }

 static int latm_parse(AVCodecParserContext *s1,
                            AVCodecContext *avctx,
                            const uint8_t **poutbuf, int *poutbuf_size,
                            const uint8_t *buf, int buf_size)
 {
     LATMParseContext *s = s1->priv_data;
     ParseContext *pc = &s->pc;
     int next;

    // s1->fetch_timestamp = 0;
    // av_log(NULL,AV_LOG_INFO,"++++[latm_parse] (1) buf_size=%d \n", buf_size);
     if(s1->flags & PARSER_FLAG_COMPLETE_FRAMES){
        // av_log(NULL,AV_LOG_INFO,"++++[latm_parse] (2) PARSER_FLAG_COMPLETE_FRAMES \n");
         next = buf_size;
     }else{
        // av_log(NULL,AV_LOG_INFO,"++++[latm_parse] (3) buf_size=%d \n", buf_size);

         next = latm_find_frame_end(s1, buf, buf_size);

         if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
            // av_log(NULL,AV_LOG_INFO,"++++[latm_parse] (4) ff_combine_frame failed [Buff size %d \n",buf_size);
             *poutbuf = NULL;
             *poutbuf_size = 0;
             return buf_size;
         }
     }
   //  av_log(NULL,AV_LOG_INFO,"++++[latm_parse] (5) buf_size=%d \n", buf_size);
     *poutbuf = buf;
     *poutbuf_size = buf_size;
     return next;
 }


 static int latm_split(AVCodecContext *avctx,
                            const uint8_t *buf, int buf_size)
 {
     int i;
     uint32_t state= -1;

     for(i=0; i<buf_size; i++){
         state= (state<<8) | buf[i];
         if((state & LATM_MASK) == LATM_HEADER)
             return i-2;
     }
     return 0;
 }


 AVCodecParser latm_parser = {
     { CODEC_ID_AAC_LATM },
     sizeof(LATMParseContext),
     NULL,
     latm_parse,
     ff_parse_close,
     latm_split,
 };

