/**********************************************************
 Software developed by AVA ( Ava Group of the University of Cordoba, ava  at uco dot es)
 Main author Rafael Munoz Salinas (rmsalinas at uco dot es)
 This software is released under BSD license as expressed below
-------------------------------------------------------------------
Copyright (c) 2013, AVA ( Ava Group University of Cordoba, ava  at uco dot es)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:

   This product includes software developed by the Ava group of the University of Cordoba.

4. Neither the name of the University nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY AVA ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL AVA BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****************************************************************/

/*** modified by Sebastien Roy, Universite de Montreal ***/

#ifndef PICAM_H
#define PICAM_H

#include <mmal/mmal.h>
#include <mmal/util/mmal_connection.h>
#include <string>

namespace picam {

  
  /**Image formats
   */
    enum RASPICAM_FORMAT{
	  RASPICAM_FORMAT_YUV420,
	  RASPICAM_FORMAT_GRAY,
	  RASPICAM_FORMAT_BGR,
	  RASPICAM_FORMAT_RGB,
	  RASPICAM_FORMAT_IGNORE //do not use
    };

	  /**Exposure types
	   */
    enum RASPICAM_EXPOSURE {
        RASPICAM_EXPOSURE_OFF,
        RASPICAM_EXPOSURE_AUTO,
        RASPICAM_EXPOSURE_NIGHT,
        RASPICAM_EXPOSURE_NIGHTPREVIEW,
        RASPICAM_EXPOSURE_BACKLIGHT,
        RASPICAM_EXPOSURE_SPOTLIGHT,
        RASPICAM_EXPOSURE_SPORTS,
        RASPICAM_EXPOSURE_SNOW,
        RASPICAM_EXPOSURE_BEACH,
        RASPICAM_EXPOSURE_VERYLONG,
        RASPICAM_EXPOSURE_FIXEDFPS,
        RASPICAM_EXPOSURE_ANTISHAKE,
        RASPICAM_EXPOSURE_FIREWORKS
    }  ;

    /**Auto white balance types
     */
    enum RASPICAM_AWB {
        RASPICAM_AWB_OFF,
        RASPICAM_AWB_AUTO,
        RASPICAM_AWB_SUNLIGHT,
        RASPICAM_AWB_CLOUDY,
        RASPICAM_AWB_SHADE,
        RASPICAM_AWB_TUNGSTEN,
        RASPICAM_AWB_FLUORESCENT,
        RASPICAM_AWB_INCANDESCENT,
        RASPICAM_AWB_FLASH,
        RASPICAM_AWB_HORIZON
    }  ;

    /**Image effects
     */
    enum RASPICAM_IMAGE_EFFECT {
        RASPICAM_IMAGE_EFFECT_NONE,
        RASPICAM_IMAGE_EFFECT_NEGATIVE,
        RASPICAM_IMAGE_EFFECT_SOLARIZE,
        RASPICAM_IMAGE_EFFECT_SKETCH,
        RASPICAM_IMAGE_EFFECT_DENOISE,
        RASPICAM_IMAGE_EFFECT_EMBOSS,
        RASPICAM_IMAGE_EFFECT_OILPAINT,
        RASPICAM_IMAGE_EFFECT_HATCH,
        RASPICAM_IMAGE_EFFECT_GPEN,
        RASPICAM_IMAGE_EFFECT_PASTEL,
        RASPICAM_IMAGE_EFFECT_WATERCOLOR,
        RASPICAM_IMAGE_EFFECT_FILM,
        RASPICAM_IMAGE_EFFECT_BLUR,
        RASPICAM_IMAGE_EFFECT_SATURATION,
        RASPICAM_IMAGE_EFFECT_COLORSWAP,
        RASPICAM_IMAGE_EFFECT_WASHEDOUT,
        RASPICAM_IMAGE_EFFECT_POSTERISE,
        RASPICAM_IMAGE_EFFECT_COLORPOINT,
        RASPICAM_IMAGE_EFFECT_COLORBALANCE,
        RASPICAM_IMAGE_EFFECT_CARTOON
    }  ;

    /**Metering types
     */
    enum RASPICAM_METERING {
        RASPICAM_METERING_AVERAGE,
        RASPICAM_METERING_SPOT,
        RASPICAM_METERING_BACKLIT,
        RASPICAM_METERING_MATRIX
    }  ;


        /// struct contain camera settings
        struct MMAL_PARAM_COLOURFX_T
        {
            int enable,u,v;       /// Turn colourFX on or off, U and V to use
        } ;
        struct PARAM_FLOAT_RECT_T
        {
            double x,y,w,h;
        } ;


        /** Structure containing all state information for the current run
         */
        struct RASPIVID_STATE
        {
            int width;                          /// Requested width of image
            int height;                         /// requested height of image
            int framerate;                      /// Requested frame rate (fps)
            /// the camera output or the encoder output (with compression artifacts)
            MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
            MMAL_POOL_T *video_pool; /// Pointer to the pool of buffers used by encoder output port
            //camera params
            int sharpness;             /// -100 to 100
            int contrast;              /// -100 to 100
            int brightness;            ///  0 to 100
            int saturation;            ///  -100 to 100
            int ISO;                   ///  TODO : what range?
            bool videoStabilisation;    /// 0 or 1 (false or true)
            int exposureCompensation;  /// -10 to +10 ?
            int shutterSpeed;
	    RASPICAM_FORMAT captureFtm;
            RASPICAM_EXPOSURE rpc_exposureMode;
            RASPICAM_METERING rpc_exposureMeterMode;
            RASPICAM_AWB rpc_awbMode;
            RASPICAM_IMAGE_EFFECT rpc_imageEffect;
            MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imageEffectsParameters;
            MMAL_PARAM_COLOURFX_T colourEffects;
            int rotation;              /// 0-359
            int hflip;                 /// 0 or 1
            int vflip;                 /// 0 or 1
            PARAM_FLOAT_RECT_T  roi;   /// region of interest to use on the sensor. Normalised [0,1] values in the rect
        } ;

        //clean buffer
        template<typename T>
        class membuf{
            public:
            membuf() {
                data=0;
                size=0;
            }
            ~membuf() {
                if ( data!=0 ) delete []data;
            }
            void resize ( size_t s ) {
                if ( s!=size ) {
                    delete data;
                    size=s;
                    data=new  T[size];
                }
            }
            T *data;
            size_t size;
        };


        /**Base class that do all the hard work
         */
        class camera
        {
            /** Struct used to pass information in encoder port userdata to callback
            */
            struct PORT_USERDATA
            {
                PORT_USERDATA() {
                    //wantToGrab=false;
                    pstate=0;
                }
                //void waitForFrame() {
                    //mutex.Lock();
                    //wantToGrab=true;
                    //mutex.Unlock();
                    //Thcond.Wait();
                //};



                RASPIVID_STATE *pstate;            /// pointer to our state in case required in callback
		void (*process)(void *plugin,unsigned char *data,int len,int64_t pts);
		void *plugin;
                //Mutex mutex;
                //ThreadCondition Thcond;
                //bool wantToGrab;
                //membuf<unsigned char> _buffData;
            };

            public:

            /**Constructor */
            camera();
            /**Destructor */
            ~camera();
            /**Opens the camera and start capturing
            */
            bool open();
            /**indicates if camera is open
            */
            bool isOpened() const
            {
                return _isOpened;
            }
            /**Starts camera capture
             */
            bool startCapture(void *plugin,void (*process)(void *plugin,unsigned char *data,int len,int64_t pts));
            /**Indicates if is capturing
             */
            bool isCapturing() const{return _isCapturing;}
            /**Grabs the next frame and keeps it in internal buffer. Blocks until next frame arrives
            */
            //bool grab();
            /**Retrieves the buffer previously grabbed.
            * NOTE: Change in version 0.0.5. Format is stablished in setFormat function
            * So type param is ignored. Do not use this parameter.
            * You can use getFormat to know the current format
             */
            //void retrieve ( unsigned char *data,RASPICAM_FORMAT type=RASPICAM_FORMAT_IGNORE );
            /**Alternative to retrieve. Returns a pointer to the original image data buffer.
              * Be careful, if you call grab(), this will be rewritten with the new data
             */
            //unsigned char *getImageBufferData() const;
            /**
             * Returns the size of the buffer returned in getImagePtr. If is like calling getImageTypeSize(getFormat()). Just for dummies :P
             */
            size_t getImageBufferSize() const;

            /** Stops camera and free resources
            */
            void close();

            //sets capture format. Can not be changed once camera is opened
            void setFormat ( RASPICAM_FORMAT fmt );
            void setWidth ( unsigned int width ) ;
            void setHeight ( unsigned int height );
            void setCaptureSize ( unsigned int width, unsigned int height );
            void setBrightness ( unsigned int brightness );
            void setRotation ( int rotation );
            void setISO ( int iso );
            void setSharpness ( int sharpness );
            void setContrast ( int contrast );
            void setSaturation ( int saturation );
            void setExposure ( RASPICAM_EXPOSURE exposure );
            void setVideoStabilization ( bool v );
            void setExposureCompensation ( int val ); //-10,10
            void setAWB ( RASPICAM_AWB awb );
            void setImageEffect ( RASPICAM_IMAGE_EFFECT imageEffect );
            void setMetering ( RASPICAM_METERING metering );
            void setHorizontalFlip ( bool hFlip );
            void setVerticalFlip ( bool vFlip );
            /**
              *Set the shutter speed to the specified value (in microseconds).
              *There is currently an upper limit of approximately 330000us (330ms, 0.33s) past which operation is undefined.
              */
            void setShutterSpeed ( unsigned int shutter ); //currently not  supported

            RASPICAM_FORMAT  getFormat() const {return State.captureFtm;}
            //Accessors
            unsigned int getWidth() const
            {
                return State.width;
            }
            unsigned int getHeight() const
            {
                return State.height;
            }
            unsigned int getBrightness() const
            {
                return State.brightness;
            }
            unsigned int getRotation() const
            {
                return State.rotation;
            }
            int getISO() const
            {
                return State.ISO;
            }
            int getSharpness() const
            {
                return State.sharpness;
            }
            int getContrast() const
            {
                return State.contrast;
            }
            int getSaturation() const
            {
                return State.saturation;
            }
            int getShutterSpeed() const
            {
                return State.shutterSpeed;
            }
            RASPICAM_EXPOSURE getExposure() const
            {
                return State.rpc_exposureMode;
            }
            RASPICAM_AWB getAWB() const
            {
                return State.rpc_awbMode;
            }
            RASPICAM_IMAGE_EFFECT getImageEffect() const
            {
                return State.rpc_imageEffect;
            }
            RASPICAM_METERING getMetering() const
            {
                return State.rpc_exposureMeterMode;
            }
            bool isHorizontallyFlipped() const
            {
                return State.hflip;
            }
            bool isVerticallyFlipped() const
            {
                return State.vflip;
            }


            //Returns an id of the camera. We assume the camera id is the one of the raspberry
            //the id is obtained using raspberry serial number obtained in /proc/cpuinfo
            std::string getId() const;

            /**Returns the size of the required buffer for the different image types in retrieve
             */
            size_t getImageTypeSize ( RASPICAM_FORMAT type ) const;

            private:
            static void video_buffer_callback ( MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer );
            void setDefaultStateParams();
            MMAL_COMPONENT_T *create_camera_component ( RASPIVID_STATE *state );
            void destroy_camera_component ( RASPIVID_STATE *state );


            //Commit
            void commitParameters( );
            void commitBrightness();
            void commitRotation() ;
            void commitISO() ;
            void commitSharpness();
            void commitContrast();
            void commitSaturation();
            void commitExposure();
            void commitAWB();
            void commitImageEffect();
            void commitMetering();
            void commitFlips();
            void commitExposureCompensation();
            void commitVideoStabilization();
            void commitShutterSpeed();


            MMAL_PARAM_EXPOSUREMODE_T convertExposure ( RASPICAM_EXPOSURE exposure ) ;
            MMAL_PARAM_AWBMODE_T  convertAWB ( RASPICAM_AWB awb ) ;
            MMAL_PARAM_IMAGEFX_T convertImageEffect ( RASPICAM_IMAGE_EFFECT imageEffect ) ;
            MMAL_PARAM_EXPOSUREMETERINGMODE_T convertMetering ( RASPICAM_METERING metering ) ;
            int convertFormat ( RASPICAM_FORMAT fmt ) ;


            //Color conversion
	    void convertBGR2RGB(unsigned char *  in_bgr,unsigned char *  out_rgb,int size);
            float VIDEO_FRAME_RATE_NUM;
            RASPIVID_STATE State;
            MMAL_STATUS_T status;
            MMAL_PORT_T *camera_video_port;//,*camera_still_port
            PORT_USERDATA callback_data;
            bool _isOpened;
            bool _isCapturing;


        };





};

#endif


