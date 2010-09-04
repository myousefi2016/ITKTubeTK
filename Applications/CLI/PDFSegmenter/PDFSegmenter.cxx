#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif

#include "itkPDFSegmenter.h"

#include "PDFSegmenterCLP.h"

#include "itkOrientedImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTimeProbesCollectorBase.h"

// Description:
// Get the PixelType and ComponentType from fileName
void GetImageType (std::string fileName,
                   itk::ImageIOBase::IOPixelType &pixelType,
                   itk::ImageIOBase::IOComponentType &componentType)
{
  typedef itk::Image<short, 3> ImageType;
  itk::ImageFileReader<ImageType>::Pointer imageReader =
        itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(fileName.c_str());
  imageReader->UpdateOutputInformation();

  pixelType = imageReader->GetImageIO()->GetPixelType();
  componentType = imageReader->GetImageIO()->GetComponentType();
}

// Description:
// Get the PixelTypes and ComponentTypes from fileNames
void GetImageTypes (std::vector<std::string> fileNames,
                    std::vector<itk::ImageIOBase::IOPixelType> &pixelTypes,
                    std::vector<itk::ImageIOBase::IOComponentType> &componentTypes)
{
  pixelTypes.clear();
  componentTypes.clear();

  // For each file, find the pixel and component type
  for (std::vector<std::string>::size_type i = 0; i < fileNames.size(); i++)
    {
    itk::ImageIOBase::IOPixelType pixelType;
    itk::ImageIOBase::IOComponentType componentType;
    GetImageType (fileNames[i],
                  pixelType,
                  componentType);
    pixelTypes.push_back(pixelType);  
    componentTypes.push_back(componentType);  
    }
}

template <class T, unsigned int N>
int DoIt( int argc, char *argv[] )
{
  PARSE_ARGS;

  itk::TimeProbesCollectorBase timeCollector;

  typedef T                                        InputPixelType;
  typedef itk::OrientedImage< InputPixelType, 3 >  InputImageType;
  typedef itk::OrientedImage< unsigned short, 3 >  MaskImageType;
  typedef itk::OrientedImage< float, 3 >           ProbImageType;

  typedef itk::ImageFileReader< InputImageType >   ImageReaderType;
  typedef itk::ImageFileReader< MaskImageType >    MaskReaderType;
  typedef itk::ImageFileWriter< MaskImageType >    MaskWriterType;
  typedef itk::ImageFileWriter< ProbImageType >    ProbImageWriterType;

  typedef itk::PDFSegmenter< InputImageType, N, MaskImageType >   PDFSegmenterType;

  typename PDFSegmenterType::Pointer pdfSegmenter = PDFSegmenterType::New();

  timeCollector.Start("LoadData");

  typename ImageReaderType::Pointer reader;
  int j = N;
  if( useTexture )
    {
    --j;
    }
  for(int i=0; i<j; i++)
    {
    reader = ImageReaderType::New();
    if(i == 0)
      {
      reader->SetFileName( inputVolume1.c_str() );
      reader->Update();
      pdfSegmenter->SetInputVolume1( reader->GetOutput() );
      }
    else if(i == 1)
      {
      reader->SetFileName( inputVolume2.c_str() );
      reader->Update();
      pdfSegmenter->SetInputVolume2( reader->GetOutput() );
      }
    else if(i == 2)
      {
      reader->SetFileName( inputVolume3.c_str() );
      reader->Update();
      pdfSegmenter->SetInputVolume3( reader->GetOutput() );
      }
    else
      {
      std::cout << "ERROR: current command line xml file limits"
                << " this filter to 3 input images" << std::endl;
      return 1;
      }
    }

  MaskReaderType::Pointer  inMaskReader = MaskReaderType::New();
  inMaskReader->SetFileName( labelmap.c_str() );
  inMaskReader->Update();
  pdfSegmenter->SetLabelmap( inMaskReader->GetOutput() );

  timeCollector.Stop("LoadData");

  pdfSegmenter->SetObjectId( objectId[0] );
  if( objectId.size() > 1 )
    {
    for( unsigned int o=1; o<objectId.size(); o++ )
      {
      pdfSegmenter->AddObjectId( objectId[o] );
      }
    }
  pdfSegmenter->SetVoidId( voidId );
  pdfSegmenter->SetUseTexture( useTexture );
  pdfSegmenter->SetErodeRadius( erodeRadius );
  pdfSegmenter->SetHoleFillIterations( holeFillIterations );
  pdfSegmenter->SetFprWeight( fprWeight );
  pdfSegmenter->SetProbabilitySmoothingStandardDeviation( 
    probSmoothingStdDev );
  pdfSegmenter->SetDraft( draft );
  pdfSegmenter->SetReclassifyNotObjectMask( reclassifyNotObjectMask );
  pdfSegmenter->SetReclassifyObjectMask( reclassifyObjectMask );

  pdfSegmenter->Update();

  timeCollector.Start("Save");

  if( pdfSegmenter->GetProbabilityImage(0) != NULL )
    {
    if( probabilityVolume.size() > 2 )
      {
      ProbImageWriterType::Pointer probImageWriter =
        ProbImageWriterType::New();
      probImageWriter->SetFileName( probabilityVolume.c_str() );
      probImageWriter->SetInput( *(pdfSegmenter->GetProbabilityImage(0)) );
      probImageWriter->Update();
      }
    }
  
  MaskWriterType::Pointer writer = MaskWriterType::New();
  writer->SetFileName( outputVolume.c_str() );
  writer->SetInput( pdfSegmenter->GetLabelmap() );
  writer->Update();

  timeCollector.Stop("Save");

  timeCollector.Report();

  return 0;
}


int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType pixelType;
  itk::ImageIOBase::IOComponentType componentType;
 
  try
    {
    GetImageType (inputVolume1, pixelType, componentType); 

    int N = 1;
    if(inputVolume2.length() > 1)
      {
      ++N;
      if(inputVolume3.length() > 1)
        {
        ++N;
        }
      }
    if(useTexture)
      {
      ++N;
      }

    switch (componentType)
      {
      case itk::ImageIOBase::UCHAR:
        if(N == 1)
          {
          return DoIt<unsigned char, 1>( argc, argv );
          }
        else if(N == 2)
          {
          return DoIt<unsigned char, 2>( argc, argv );
          }
        else if(N == 3)
          {
          return DoIt<unsigned char, 3>( argc, argv );
          }
        else 
          {
          return DoIt<unsigned char, 4>( argc, argv );
          }
        break;
      case itk::ImageIOBase::USHORT:
        if(N == 1)
          {
          return DoIt<unsigned short, 1>( argc, argv );
          }
        else if(N == 2)
          {
          return DoIt<unsigned short, 2>( argc, argv );
          }
        else if(N == 3) 
          {
          return DoIt<unsigned short, 3>( argc, argv );
          }
        else 
          {
          return DoIt<unsigned short, 4>( argc, argv );
          }
        break;
      case itk::ImageIOBase::CHAR:
      case itk::ImageIOBase::SHORT:
        if(N == 1)
          {
          return DoIt<short, 1>( argc, argv );
          }
        else if(N == 2)
          {
          return DoIt<short, 2>( argc, argv );
          }
        else if(N == 3) 
          {
          return DoIt<short, 3>( argc, argv );
          }
        else 
          {
          return DoIt<short, 4>( argc, argv );
          }
        break;
      case itk::ImageIOBase::UINT:
      case itk::ImageIOBase::INT:
      case itk::ImageIOBase::ULONG:
      case itk::ImageIOBase::LONG:
      case itk::ImageIOBase::FLOAT:
      case itk::ImageIOBase::DOUBLE:
        if(N == 1)
          {
          return DoIt<float, 1>( argc, argv );
          }
        else if(N == 2)
          {
          return DoIt<float, 2>( argc, argv );
          }
        else if(N == 3) 
          {
          return DoIt<float, 3>( argc, argv );
          }
        else
          {
          return DoIt<float, 4>( argc, argv );
          }
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }
  catch( itk::ExceptionObject &excep)
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}

