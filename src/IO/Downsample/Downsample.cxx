/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#if defined(__EMSCRIPTEN__)
#include "itkJSONImageIO.h"
#endif
#include "itkBinShrinkImageFilter.h"
#include "itkVectorImage.h"
#include "itkResampleImageFilter.h"
#include "itkLabelImageGaussianInterpolateImageFunction.h"
#include "itkImageRegionSplitterSlowDimension.h"
#include "itkExtractImageFilter.h"
#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "itkVectorImage.h"
#include "itkOffset.h"
#include "itkVector.h"
#include "itkPoint.h"
#include "itkCovariantVector.h"
#include "itkSymmetricSecondRankTensor.h"
#include "itkDiffusionTensor3D.h"
#include <complex>
#include "itkFixedArray.h"
#include "itkArray.h"
#include "itkMatrix.h"
#include "itkVariableLengthVector.h"
#include "itkVariableSizeMatrix.h"
#include <fstream>

template < typename TImage >
int
LabelImageDownsample( char * argv [] )
{
  using ImageType = TImage;
  unsigned int isLabelImage = atoi( argv[1] );
  const char * inputImageFile = argv[2];
  const char * outputImageFile = argv[3];
  unsigned int factorI = atoi( argv[4] );
  unsigned int factorJ = atoi( argv[5] );
  unsigned int factorK = atoi( argv[6] );
  unsigned int maxTotalSplits = atoi( argv[7] );
  unsigned int split = atoi( argv[8] );
  const char * numberOfSplitsFile = argv[9];

  using ReaderType = itk::ImageFileReader< ImageType >;
  auto reader = ReaderType::New();
  reader->SetFileName( inputImageFile );

  using FilterType = itk::BinShrinkImageFilter< ImageType, ImageType >;
  auto filter = FilterType::New();
  filter->SetInput( reader->GetOutput() );
  filter->SetShrinkFactor( 0, factorI );
  filter->SetShrinkFactor( 1, factorJ );
  if (ImageType::ImageDimension > 2) {
    filter->SetShrinkFactor( 2, factorK );
  }

  using WriterType = itk::ImageFileWriter< ImageType >;
  auto writer = WriterType::New();
  writer->SetFileName( outputImageFile );

  using ResampleFilterType = itk::ResampleImageFilter< ImageType, ImageType >;
  auto resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput( reader->GetOutput() );

  filter->UpdateOutputInformation();
  using ROIFilterType = itk::ExtractImageFilter< ImageType, ImageType >;
  auto roiFilter = ROIFilterType::New();
  using RegionType = typename ImageType::RegionType;
  const RegionType largestRegion( filter->GetOutput()->GetLargestPossibleRegion() );

  using SplitterType = itk::ImageRegionSplitterSlowDimension;
  auto splitter = SplitterType::New();
  const unsigned int numberOfSplits = splitter->GetNumberOfSplits( largestRegion, maxTotalSplits );

  std::ofstream ostream(numberOfSplitsFile);
  ostream << numberOfSplits;
  ostream.close();

  if (split >= numberOfSplits)
  {
    //std::cerr << "Error: requested split: " << split << " is outside the number of splits: " << numberOfSplits << std::endl;
    split = 0;
    //return EXIT_FAILURE;
  }

  RegionType requestedRegion( largestRegion );
  splitter->GetSplit( split, numberOfSplits, requestedRegion );
  roiFilter->SetExtractionRegion( requestedRegion );
  writer->SetInput( roiFilter->GetOutput() );

  roiFilter->SetInput( resampleFilter->GetOutput() );
  const ImageType * shrunk = filter->GetOutput();
  resampleFilter->SetSize( shrunk->GetLargestPossibleRegion().GetSize() );
  resampleFilter->SetOutputOrigin( shrunk->GetOrigin() );
  auto spacing = shrunk->GetSpacing();
  resampleFilter->SetOutputSpacing( spacing );
  resampleFilter->SetOutputDirection( shrunk->GetDirection() );
  using CoordRepType = double;
  using InterpolatorType = itk::LabelImageGaussianInterpolateImageFunction< ImageType, CoordRepType >;
  auto interpolator = InterpolatorType::New();
  double sigma[ImageType::ImageDimension];
  double sigmaMax = 0.0;
  for (unsigned int dim = 0; dim < ImageType::ImageDimension; ++dim ) {
    sigma[dim] = spacing[dim] * 0.7355;
    if (sigma[dim] > sigmaMax) {
      sigmaMax = sigma[dim];
    }
  }
  interpolator->SetSigma( sigma );
  interpolator->SetAlpha( sigmaMax * 2.5 );
  resampleFilter->SetInterpolator( interpolator );

  try
  {
    writer->Update();
  }
  catch( std::exception & error )
  {
    std::cerr << "Error: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

template < typename TImage >
int
Downsample( char * argv [] )
{
  using ImageType = TImage;
  unsigned int isLabelImage = atoi( argv[1] );
  const char * inputImageFile = argv[2];
  const char * outputImageFile = argv[3];
  unsigned int factorI = atoi( argv[4] );
  unsigned int factorJ = atoi( argv[5] );
  unsigned int factorK = atoi( argv[6] );
  unsigned int maxTotalSplits = atoi( argv[7] );
  unsigned int split = atoi( argv[8] );
  const char * numberOfSplitsFile = argv[9];

  using ReaderType = itk::ImageFileReader< ImageType >;
  auto reader = ReaderType::New();
  reader->SetFileName( inputImageFile );

  using FilterType = itk::BinShrinkImageFilter< ImageType, ImageType >;
  auto filter = FilterType::New();
  filter->SetInput( reader->GetOutput() );
  filter->SetShrinkFactor( 0, factorI );
  filter->SetShrinkFactor( 1, factorJ );
  if (ImageType::ImageDimension > 2) {
    filter->SetShrinkFactor( 2, factorK );
  }

  using WriterType = itk::ImageFileWriter< ImageType >;
  auto writer = WriterType::New();
  writer->SetFileName( outputImageFile );

  using ResampleFilterType = itk::ResampleImageFilter< ImageType, ImageType >;
  auto resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput( reader->GetOutput() );

  filter->UpdateOutputInformation();
  using ROIFilterType = itk::ExtractImageFilter< ImageType, ImageType >;
  auto roiFilter = ROIFilterType::New();
  using RegionType = typename ImageType::RegionType;
  const RegionType largestRegion( filter->GetOutput()->GetLargestPossibleRegion() );

  using SplitterType = itk::ImageRegionSplitterSlowDimension;
  auto splitter = SplitterType::New();
  const unsigned int numberOfSplits = splitter->GetNumberOfSplits( largestRegion, maxTotalSplits );

  std::ofstream ostream(numberOfSplitsFile);
  ostream << numberOfSplits;
  ostream.close();

  if (split >= numberOfSplits)
  {
    //std::cerr << "Error: requested split: " << split << " is outside the number of splits: " << numberOfSplits << std::endl;
    split = 0;
    //return EXIT_FAILURE;
  }

  RegionType requestedRegion( largestRegion );
  splitter->GetSplit( split, numberOfSplits, requestedRegion );
  roiFilter->SetExtractionRegion( requestedRegion );
  writer->SetInput( roiFilter->GetOutput() );

  roiFilter->SetInput( filter->GetOutput() );

  try
  {
    writer->Update();
  }
  catch( std::exception & error )
  {
    std::cerr << "Error: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

template <typename TComponent, unsigned int VDimension>
int
PixelTypeDownsample( const itk::IOPixelEnum pixelType, char * argv[] )
{
  using ComponentType = TComponent;

  unsigned int isLabelImage = atoi( argv[1] );
  if (isLabelImage)
  {
    using PixelType = ComponentType;
    using ImageType = itk::Image<PixelType, VDimension>;
    return LabelImageDownsample<ImageType>( argv );
  }

  switch (pixelType)
  {
    case itk::IOPixelEnum::SCALAR:
    {
      using PixelType = ComponentType;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::RGB:
    {
      using PixelType = itk::RGBPixel< ComponentType >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::RGBA:
    {
      using PixelType = itk::RGBAPixel< ComponentType >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::OFFSET:
    //{
      //using PixelType = itk::Offset< VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::VECTOR:
    {
      using PixelType = itk::Vector< ComponentType, VDimension >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::POINT:
    //{
      //using PixelType = itk::Point< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::COVARIANTVECTOR:
    {
      using PixelType = itk::CovariantVector< ComponentType, VDimension >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::SYMMETRICSECONDRANKTENSOR:
    {
      using PixelType = itk::SymmetricSecondRankTensor< ComponentType, VDimension >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::DIFFUSIONTENSOR3D:
    //{
      //using PixelType = itk::DIFFUSIONTENSOR3D< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::COMPLEX:
    //{
      //using PixelType = std::complex< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::FIXEDARRAY:
    //{
      //using PixelType = itk::FixedArray< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::ARRAY:
    //{
      //using PixelType = itk::Array< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::MATRIX:
    //{
      //using PixelType = itk::Matrix< ComponentType, VDimension, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::VARIABLELENGTHVECTOR:
    {
      using ImageType = itk::VectorImage<ComponentType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::VARIABLESIZEMATRIX:
    //{
      //using ImageType = itk::VectorImage<ComponentType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}

    case itk::IOPixelEnum::UNKNOWNPIXELTYPE:
    default:
      std::cerr << "Unknown or unsupported pixel type: " << pixelType << std::endl;
      return EXIT_FAILURE;

  }
  return EXIT_SUCCESS;
}

template <typename TComponent, unsigned int VDimension>
int
PixelTypeDownsampleUIntegers( const itk::IOPixelEnum pixelType, char * argv[] )
{
  using ComponentType = TComponent;

  unsigned int isLabelImage = atoi( argv[1] );
  if (isLabelImage)
  {
    using PixelType = ComponentType;
    using ImageType = itk::Image<PixelType, VDimension>;
    return LabelImageDownsample<ImageType>( argv );
  }

  switch (pixelType)
  {
    case itk::IOPixelEnum::SCALAR:
    {
      using PixelType = ComponentType;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::RGB:
    {
      using PixelType = itk::RGBPixel< ComponentType >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::RGBA:
    {
      using PixelType = itk::RGBAPixel< ComponentType >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::OFFSET:
    //{
      //using PixelType = itk::Offset< VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::VECTOR:
    //{
      //using PixelType = itk::Vector< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::POINT:
    //{
      //using PixelType = itk::Point< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::COVARIANTVECTOR:
    //{
      //using PixelType = itk::CovariantVector< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::SYMMETRICSECONDRANKTENSOR:
    //{
      //using PixelType = itk::SymmetricSecondRankTensor< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::DIFFUSIONTENSOR3D:
    //{
      //using PixelType = itk::DIFFUSIONTENSOR3D< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::COMPLEX:
    //{
      //using PixelType = std::complex< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::FIXEDARRAY:
    //{
      //using PixelType = itk::FixedArray< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::ARRAY:
    //{
      //using PixelType = itk::Array< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::MATRIX:
    //{
      //using PixelType = itk::Matrix< ComponentType, VDimension, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::VARIABLELENGTHVECTOR:
    {
      using ImageType = itk::VectorImage<ComponentType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::VARIABLESIZEMATRIX:
    //{
      //using ImageType = itk::VectorImage<ComponentType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}

    case itk::IOPixelEnum::UNKNOWNPIXELTYPE:
    default:
      std::cerr << "Unknown or unsupported pixel type: " << pixelType << std::endl;
      return EXIT_FAILURE;

  }
  return EXIT_SUCCESS;
}

template <typename TComponent, unsigned int VDimension>
int
PixelTypeDownsampleFloats( const itk::IOPixelEnum pixelType, char * argv[] )
{
  using ComponentType = TComponent;

  unsigned int isLabelImage = atoi( argv[1] );
  if (isLabelImage)
  {
    //using PixelType = ComponentType;
    //using ImageType = itk::Image<PixelType, VDimension>;
    //return LabelImageDownsample<ImageType>( argv );
    return EXIT_FAILURE;
  }

  switch (pixelType)
  {
    case itk::IOPixelEnum::SCALAR:
    {
      using PixelType = ComponentType;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::RGB:
    //{
      //using PixelType = itk::RGBPixel< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::RGBA:
    //{
      //using PixelType = itk::RGBAPixel< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::OFFSET:
    //{
      //using PixelType = itk::Offset< VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::VECTOR:
    {
      using PixelType = itk::Vector< ComponentType, VDimension >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::POINT:
    //{
      //using PixelType = itk::Point< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::COVARIANTVECTOR:
    {
      using PixelType = itk::CovariantVector< ComponentType, VDimension >;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::SYMMETRICSECONDRANKTENSOR:
    //{
      //using PixelType = itk::SymmetricSecondRankTensor< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::DIFFUSIONTENSOR3D:
    //{
      //using PixelType = itk::DIFFUSIONTENSOR3D< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::COMPLEX:
    //{
      //using PixelType = std::complex< ComponentType >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::FIXEDARRAY:
    //{
      //using PixelType = itk::FixedArray< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::ARRAY:
    //{
      //using PixelType = itk::Array< ComponentType, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    //case itk::IOPixelEnum::MATRIX:
    //{
      //using PixelType = itk::Matrix< ComponentType, VDimension, VDimension >;
      //using ImageType = itk::Image<PixelType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}
    case itk::IOPixelEnum::VARIABLELENGTHVECTOR:
    {
      using ImageType = itk::VectorImage<ComponentType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    //case itk::IOPixelEnum::VARIABLESIZEMATRIX:
    //{
      //using ImageType = itk::VectorImage<ComponentType, VDimension>;
      //return Downsample<ImageType>( argv );
    //}

    case itk::IOPixelEnum::UNKNOWNPIXELTYPE:
    default:
      std::cerr << "Unknown or unsupported pixel type: " << pixelType << std::endl;
      return EXIT_FAILURE;

  }
  return EXIT_SUCCESS;
}

template <typename TComponent, unsigned int VDimension>
int
PixelTypeDownsampleScalar( const itk::IOPixelEnum pixelType, char * argv[] )
{
  using ComponentType = TComponent;

  unsigned int isLabelImage = atoi( argv[1] );
  if (isLabelImage)
  {
    //using PixelType = ComponentType;
    //using ImageType = itk::Image<PixelType, VDimension>;
    //return LabelImageDownsample<ImageType>( argv );
    return EXIT_FAILURE;
  }

  switch (pixelType)
  {
    case itk::IOPixelEnum::SCALAR:
    {
      using PixelType = ComponentType;
      using ImageType = itk::Image<PixelType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::VARIABLELENGTHVECTOR:
    {
      using ImageType = itk::VectorImage<ComponentType, VDimension>;
      return Downsample<ImageType>( argv );
    }
    case itk::IOPixelEnum::UNKNOWNPIXELTYPE:
    default:
      std::cerr << "Unknown or unsupported pixel type: " << pixelType << std::endl;
      return EXIT_FAILURE;

  }
  return EXIT_SUCCESS;
}

template <unsigned int VDimension>
int
ComponentTypeDownsample( const itk::IOPixelEnum pixelType, const itk::IOComponentEnum componentType, char * argv[] )
{
  switch (componentType)
  {
    case itk::IOComponentEnum::UCHAR:
    {
      using ComponentType = unsigned char;
      return PixelTypeDownsampleUIntegers<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::CHAR:
    {
      using ComponentType = char;
      return PixelTypeDownsampleScalar<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::USHORT:
    {
      using ComponentType = unsigned short;
      return PixelTypeDownsampleUIntegers<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::SHORT:
    {
      using ComponentType = short;
      return PixelTypeDownsampleScalar<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::UINT:
    {
      using ComponentType = unsigned int;
      return PixelTypeDownsampleUIntegers<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::INT:
    {
      using ComponentType = int;
      return PixelTypeDownsampleScalar<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::ULONG:
    {
      // JS does not have broad support for 64-bit ints
      //using ComponentType = unsigned long;
      using ComponentType = unsigned int;
      return PixelTypeDownsampleUIntegers<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::LONG:
    {
      // JS does not have broad support for 64-bit ints
      //using ComponentType = long;
      using ComponentType = int;
      return PixelTypeDownsampleScalar<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::FLOAT:
    {
      using ComponentType = float;
      return PixelTypeDownsampleFloats<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::DOUBLE:
    {
      using ComponentType = double;
      return PixelTypeDownsampleFloats<ComponentType, VDimension>( pixelType, argv );
    }

    case itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE:
    default:
      std::cerr << "Unknown and unsupported component type: " << componentType << std::endl;
      return EXIT_FAILURE;

  }
  return EXIT_SUCCESS;
}

int main( int argc, char * argv[] )
{
  if( argc < 10 )
    {
    std::cerr << "Usage: " << argv[0] << " <isLabelImage> <inputImage> <outputImage> <factorI> <factorJ> <factorK> <maxTotalSplits> <split> <numberOfSplitsFile>" << std::endl;
    return EXIT_FAILURE;
    }
  const char * inputImageFile = argv[2];

#if defined(__EMSCRIPTEN__)
  itk::JSONImageIO::Pointer imageIO = itk::JSONImageIO::New();
#else
  itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO( inputImageFile, itk::CommonEnums::IOFileMode::ReadMode);
#endif
  imageIO->SetFileName( inputImageFile );
  imageIO->ReadImageInformation();

  using IOComponentType = itk::IOComponentEnum;
  const IOComponentType componentType = imageIO->GetComponentType();

  using IOPixelType = itk::IOPixelEnum;
  const IOPixelType pixelType = imageIO->GetPixelType();

  const unsigned int imageDimension = imageIO->GetNumberOfDimensions();

  switch (imageDimension)
  {
  case 2:
    {
    return ComponentTypeDownsample<2>( pixelType, componentType,  argv );
    }
  case 3:
    {
    return ComponentTypeDownsample<3>( pixelType, componentType,  argv );
    }
  default:
    std::cerr << "Dimension not implemented!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
