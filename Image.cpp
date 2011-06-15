/* file:        Image.cpp
** author:      Andrea Vedaldi
** description: Definition of class Image.
**/

#include"Image.h"

#include<iostream>
#include<fstream>
#include<sstream>
#include<algorithm>
#include<iterator>

using namespace std ;

/** @class ImageException
 **
 ** @brief Exception generated by the ::Image class.
 **/

/** @fn ImageException::ImageException(const string&)
 **
 ** @brief Constructs an exception with the given message.
 ** 
 ** @param msg the message.
 **/

/** @class Image
 ** 
 ** @brief Grayscale or RGB image. 
 **
 ** The class  represent a  Grayscale or RGB  image, according  to the
 ** following list:
 **
 ** - Grayscale image (pixel type @c L): one byte for the luminance value.
 ** - Color image (pixel type @c RBG): three bytes for the
 **   R, G and B components.
 **
 ** Methods  for reading  and  writing  the image  in  PNM format  are
 ** provided.
 **/

/** @brief
 ** Input stream manipulator to remove whitespaces from a PNM data stream.
 ** from an input stream.
 **/
class _pnmws { } pnmws ;

/** @brief  Processes  the manipulator  that  removes whitespaces  and
 ** comments from a PNM data stream.
 ** 
 ** The operator discards all  whitespaces and commets from the stream
 ** until the first character that is neither a whitespace nor belongs
 ** to a comment.  Accroding to  the PNM standard, whitespaces are LF,
 ** CR,  tabs  and comments  are  any  string  introduced by  '#'  and
 ** terminating in LF.
 **
 ** @param is input stream.
 ** @param _pnmws input stream manipulator.
 ** @return the input stream.
 **/
istream& 
operator>>(istream& is, const _pnmws&)
{
  char c ;
  bool stop = false ;
  while(!stop) {
    
    switch(c = is.get()) {

    case ' ' : case '\t' : 
    case '\n' : case '\r' :
      // Drop this whitespace
      break ;

    case '#' :
      // Remove the comment
      {
        bool stopcomment = false ;
        while(!stopcomment) {
          switch(c = is.get()) {         
          case '\r' : case '\n' : case EOF :
            // The comment terminates here
            stopcomment = true ;
            break ;
            
          default:
            break ;
          }
        }
      }
      break ;
      
    default:
      // The character is not a whitespace: put it back.
      is.putback(c) ;
    case EOF:
      stop = true ;
      break ;
    }
  }
  return is ;
}
 

/** @brief Constructs an image from a PNM data stream.
 **
 ** A PNM file  is either a PPM (color image),  PGM (gray scale image)
 ** or PBM (black and white  image).  Currently only PPM and PGM files
 ** are supported. A PPM file generates a image of type @c Image::RGB,
 ** while a PGM file generates an image of type @c Image::L.
 **
 ** @param is the input stream.
 ** @param I the output image.
 ** @return the stream @a is.
 **/
istream& operator>>(istream& is, Image& I)
{
  if(I.pt) {
    delete [] I.pt ;
    I.width = 0 ;
    I.height = 0 ;
  }
    
  // Accroding to the specification, the first character has to be 'P'
  if(is.get() != 'P') {
    throw ImageException("The file does not seem to be in PNM format.") ;
  }

  // The second character specifies the version of the file
  int v ;
  is>>v ;
  if(!is) { 
    throw ImageException("The file does not seem to be in PNM format.") ;
  }

  // There are four format supported: PGM ASCII/BINARY and PPM ASCII/BINARY.
  // According to the file type, a grayscale or color image is created.
  switch(v) {
  case 2: case 5:
    // PGM Format
    I.type = Image::L ;
    break ;
    
  case 3: case 6:
    // PPM Format
    I.type = Image::RGB ;
    break ;
    
  case 1: case 4:
    throw ImageException("PBM format not supported") ;
  
  default:
    throw ImageException("The file does not seem to be in PPM/PGM format.") ;
  }

  // The next three fields are: width, height and maximum pixel value
  int maxval ;
  is>>pnmws>>I.width ;
  is>>pnmws>>I.height ;
  is>>pnmws>>maxval ;

  if(!is) {
    throw ImageException("The PNM file seems to be corrupted.") ;
  }
  
  if (v != 5 && v != 6)
    // ASCII types may have comments and whitespace
    is>>pnmws ;
  else {
    // Binary types may have only a newline before the data
    // blob.
    unsigned char ch = is.get() ;
    if (ch != 10 && ch != 13)
      is.putback(ch);
  }
  
  // Now we are ready to read the data
  I.pt = new unsigned char [I.getDataSize()] ;

  int number ;
  float scale = 256.0f / maxval ;
  
  for(int y = I.height-1 ; y >= 0 ; --y) {
    for(int x = 0 ; x <  I.width ; ++x) {
      unsigned char* pt = I.getPixelPt(x,y) ;
      switch(v) {
      case 5 :
        // Binary-encoded PGM
        *pt = is.get() ;
        break ;
        
      case 6 :
        // Binary-encoded PPM
        *pt++ = is.get() ;
        *pt++ = is.get() ;
        *pt   = is.get() ;
        break ;

      case 2 :
        // ASCII-encoded PGM
        is>>number ;
        *pt = (unsigned char)( number *  scale ) ;
        break ;

      case 3:
        // ASCII-encoded PPM
        is>>number ; *pt++ = (unsigned char)( number *  scale ) ;
        is>>number ; *pt++ = (unsigned char)( number *  scale ) ;
        is>>number ; *pt   = (unsigned char)( number *  scale ) ;
        break ;
      }
    }
  }
  return is ;
}

/** @brief Encodes an image into PNM data stream.
 **
 ** An imae of type @c Image::RGB generates a PPM file, while an image
 ** of type @c Image::L generates a PGM file.
 **
 ** @param os the output stream
 ** @param I the encoded image.
 ** @return the stream @a os.
 **/
ostream& operator<<(ostream& os, const Image& I)
{
  os<<'P'
    <<((I.getPixelType() == Image::L)?'5':'6')<<endl
    <<I.getWidth()<<' '<<I.getHeight()<<" 255"<<endl ;
    
  for(int y = I.getHeight()-1 ; y >= 0 ; --y) {
    for(int x = 0 ; x <  I.getWidth() ; ++x) {
      const unsigned char* pt = I.getPixelPt(x,y) ;
      switch(I.getPixelType()) {
      case Image::L :
        os.put(*pt) ;
        break ;
        
      case Image::RGB :
        os.put(*pt++) ; 
        os.put(*pt++) ;
        os.put(*pt) ;
        break ;
      }
    }
  }
  return os<<flush ;
}


/** @brief Constructs an empty image.
 **/
Image::Image() 
  : type(RGB), pt(NULL), width(0), height(0) 
{ }

/** @brief Constructs an image of the specified type and size.
 **
 ** @param _pixel Pixel format.
 ** @param _width Width of the image [pixels].
 ** @param _height Height of the image [pixels].
 **/
Image::Image(PixelType _pixel, int _width, int _height) 
  : type(_pixel), pt(NULL), width(_width), height(_height) 
{ 
  pt = new unsigned char [getDataSize()] ;
}

/** @brief Destroys an image freeing the data buffer.
 **/
Image::~Image() 
{ 
  if(pt) delete [] pt ; 
}

/** @brief Constructs an image by copying the specified image.
 **
 ** @param I image to copy from.
 **/
Image::Image(const Image& I) 
  : type(I.getPixelType()), width(I.getWidth()), height(I.getHeight()) 
{
  pt = new unsigned char [getDataSize()] ;
  
  for(int k = 0 ; k < getDataSize() ; ++k) 
    *(getDataPt() + k) = *(I.getDataPt() + k) ;
  
  putDebug(cerr) ;
}

/** @brief Copies the specifeid image.
 **
 ** @param I image to copy from.
 ** @return this image.
 **/
Image& 
Image::operator=(const Image& I) 
{
  if(pt) {
    delete [] pt ;
    width = 0 ;
    height = 0 ;
  }

  type = I.getPixelType() ;
  width = I.getWidth() ; 
  height = I.getHeight() ; 

  pt = new unsigned char [getDataSize()] ;
  
  for(int k = 0 ; k < getDataSize() ; ++k) 
    *(getDataPt() + k) = *(I.getDataPt() + k) ;
  
  putDebug(cerr); 

  return *this ;
}

/** @breif Puts the symbolic name of a pixel type on a stream.
 **
 ** @param os output stream
 ** @param type pixel type code.
 ** @return the stream @c os.
 **/
std::ostream& 
operator<<(std::ostream& os, Image::PixelType type) 
{
  switch(type) {
  case Image::L : return os<<"L" ;
  case Image::RGB : return os<<"RGB" ;
  default: assert(0) ; return os ;
  }
}

/** @brief Puts debugging informations on a stream.
 **
 ** @param os output stream.
 ** @return the stream @a os.
 **/
ostream&
Image::putDebug(ostream& os) const
{
  return 
    os<<"[Image t:"<<type
      <<" w:"<<width
      <<" h:"<<height
      <<"]" ;
}

/** @enum Image::PixelType
 ** 
 ** Pixel format: can be either one byte for the luminance component
 ** (@c L ) or three bytes (@c RBG ) for the RGB components.
 **/

/** @fn Image::getDataPt()
 **
 ** @brief Returns a pointer to the data.
 **
 ** @sa getDataPt() const
 **/

/** @fn Image::getDataPt() const
 **
 ** @brief Returns a constant pointer to the data.
 **
 ** @sa getDataPt() const
 **/

/** @fn Image::getDataSize() const
 **
 ** @brief Returns the size of the image data buffer.
 **
 ** @return size of the buffer [bytes].
 **/

/** @fn Image::getWidth() const
 **
 ** @brief Returns the width of the image.
 **
 ** @return width of the image [pixels].
 **
 ** @sa getHeight() const
 **/

/** @fn Image::getHeight() const
 **
 ** @brief Returns the height of the image.
 **
 ** @return height of the image [pixels].
 **
 ** @sa getWidth() const
 **/

/** @fn Image::getPixelType() const
 **
 ** @brief Returns the format of the pixels.
 **
 ** @return format of the pixels.
 **
 ** @sa getPixelSize() const
 **/

/** @fn Image::getPixelSize() const
 **
 ** @brief Returns the number of bytes for each pixel.
 **
 ** @return size of a pixel [bytes].
 **
 ** @sa getPixelType() const
 **/

/** @fn Image::getPixelPt(int,int)
 **
 ** @brief Returns a pointer to the data of the specified pixel.
 **
 ** @param x X coordinate of the pixel.
 ** @param y Y coordinate of the pixel.
 ** @return reference to the pixel.
 **
 ** @sa Image::getPixelPt(int,int) const
 **/

/** @fn Image::getPixelPt(int,int) const
 **
 ** @brief Returns a pointer to the constant data of the specified pixel.
 **
 ** @param x X coordinate of the pixel.
 ** @param y Y coordinate of the pixel.
 ** @return constant reference to the pixel.
 **
 ** @sa Image::getPixelPt(int,int)
 **/

