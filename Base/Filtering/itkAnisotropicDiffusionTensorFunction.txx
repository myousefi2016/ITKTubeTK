/*=========================================================================

Library:   TubeTK

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/
#ifndef __itkAnisotropicDiffusionTensorFunction_txx
#define __itkAnisotropicDiffusionTensorFunction_txx

#include "itkAnisotropicDiffusionTensorFunction.h"

namespace itk {

template< class TImageType >
AnisotropicDiffusionTensorFunction< TImageType>
::AnisotropicDiffusionTensorFunction()
{
  typename Superclass::RadiusType r;
  r.Fill( 1 );
  this->SetRadius( r );

  // Dummy neighborhood.
  NeighborhoodType it;
  it.SetRadius( r );

  // Find the center index of the neighborhood.
  m_Center = static_cast< unsigned int >( it.Size() / 2 );

  // Get the stride length for each axis.
  for( unsigned int i = 0; i < ImageDimension; i++ )
    {
    m_xStride[i] = static_cast< unsigned int >( it.GetStride(i) );
    }

  // Calculate the required indexes surrounding the center position once.
  for( unsigned int i = 0; i < ImageDimension; i++ )
    {
    m_positionA[i] = m_Center + m_xStride[i];
    m_positionB[i] = m_Center - m_xStride[i];
    for( unsigned int j = i+1; j < ImageDimension; j++ )
      {
      m_positionAa[i][j] = m_Center - m_xStride[i] - m_xStride[j];
      m_positionBa[i][j] = m_Center - m_xStride[i] + m_xStride[j];
      m_positionCa[i][j] = m_Center + m_xStride[i] - m_xStride[j];
      m_positionDa[i][j] = m_Center + m_xStride[i] + m_xStride[j];
      }
    }

  // Whether or not to use the image spacing and direction when computing the
  // derivatives
  this->m_UseImageSpacing = true;
  this->m_UseImageDirection = true;
}

template <class TImageType>
void
AnisotropicDiffusionTensorFunction<TImageType>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template< class TImageType >
typename AnisotropicDiffusionTensorFunction< TImageType >::PixelType
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeUpdate(const NeighborhoodType &neighborhood,
                void *globalData,
                const FloatOffsetType& offset)
{
  DiffusionTensorNeighborhoodType tensorNeighborhood;
  SpacingType                     spacing;
  return this->ComputeUpdate( neighborhood,
                              tensorNeighborhood,
                              spacing,
                              globalData,
                              offset );
}

template< class TImageType >
typename AnisotropicDiffusionTensorFunction< TImageType >::PixelType
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeUpdate(const NeighborhoodType &neighborhood,
                const DiffusionTensorNeighborhoodType &tensorNeighborhood,
                const SpacingType &spacing,
                void *globalData,
                const FloatOffsetType& itkNotUsed(offset) )
{
  // Global data structure
  GlobalDataStruct *gd = (GlobalDataStruct *)globalData;

  // Compute the first and 2nd derivative for the intensity images
  this->ComputeIntensityFirstAndSecondOrderPartialDerivatives( neighborhood,
                                                               spacing,
                                                               gd );

  // Compute the diffusion tensor matrix first derivatives if not provided
  this->ComputeDiffusionTensorFirstOrderPartialDerivatives( tensorNeighborhood,
                                                            spacing,
                                                            gd );

  // Compute the update term
  return this->ComputeFinalUpdateTerm( tensorNeighborhood, gd );
}

template< class TImageType >
typename AnisotropicDiffusionTensorFunction< TImageType >::PixelType
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeUpdate(const NeighborhoodType &neighborhood,
                const DiffusionTensorNeighborhoodType &tensorNeighborhood,
                const TensorDerivativeImageRegionType &tensorDerivativeRegion,
                const SpacingType &spacing,
                void *globalData,
                const FloatOffsetType& itkNotUsed(offset) )
{
  // Global data structure
  GlobalDataStruct *gd = (GlobalDataStruct *)globalData;

  // Compute the first and 2nd derivative for the intensity images
  this->ComputeIntensityFirstAndSecondOrderPartialDerivatives( neighborhood,
                                                               spacing,
                                                               gd );

  // We are provided the diffusion tensor matrix first derivatives, so
  // copy them into the global data struct
  this->CopyTensorDerivativeToGlobalData( tensorDerivativeRegion, gd );

  // Compute the update term
  return this->ComputeFinalUpdateTerm( tensorNeighborhood, gd );
}

template< class TImageType >
void
AnisotropicDiffusionTensorFunction< TImageType >
::CopyTensorDerivativeToGlobalData(
    const TensorDerivativeImageRegionType &tensorDerivativeRegion,
    GlobalDataStruct *gd) const
{
  assert( gd );
  TensorDerivativeType derivativePixel = tensorDerivativeRegion.Get();
  for( unsigned int i = 0; i < ImageDimension; i++ )
    {
    for( unsigned int j = 0; j < ImageDimension; j++ )
      {
      gd->m_DT_dxy[i][j] = derivativePixel(i, j);
      }
    }
}

template< class TImageType >
typename AnisotropicDiffusionTensorFunction< TImageType >::TensorDerivativeType
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeDiffusionTensorFirstOrderPartialDerivatives(
    const DiffusionTensorNeighborhoodType &tensorNeighborhood,
    const SpacingType &spacing,
    GlobalDataStruct *gd) const
{
  TensorDerivativeType TensorDerivative;
  typename SpacingType::ComponentType space_i;

  for( unsigned int i = 0; i < ImageDimension; i++ )
    {
    DiffusionTensorType positionA_Tensor_value
        = tensorNeighborhood.GetPixel( m_positionA[i] );
    DiffusionTensorType positionB_Tensor_value
        = tensorNeighborhood.GetPixel( m_positionB[i] );
    space_i = 0.5 / spacing[i];

    for( unsigned int j = 0; j < ImageDimension; j++ )
      {
      TensorDerivative(i,j)
          = space_i * ( positionA_Tensor_value(i,j)
                        - positionB_Tensor_value(i,j) );
      if( gd )
        {
        gd->m_DT_dxy[i][j] = TensorDerivative(i,j);
        }
      }
    }
  return TensorDerivative;
}

template< class TImageType >
void
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeDiffusionTensorFirstOrderPartialDerivatives(
    const DiffusionTensorNeighborhoodType &tensorNeighborhood,
    TensorDerivativeImageRegionType &tensorDerivativeRegion,
    const SpacingType &spacing ) const
{
  tensorDerivativeRegion.Set(
      this->ComputeDiffusionTensorFirstOrderPartialDerivatives(
          tensorNeighborhood, spacing, NULL ) );
}

template< class TImageType >
void
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeIntensityFirstAndSecondOrderPartialDerivatives(
    const NeighborhoodType &neighborhood,
    const SpacingType &spacing,
    GlobalDataStruct *gd) const
{
  assert( gd );
  gd->m_GradMagSqr = 1.0e-6;
  const ScalarValueType center_value = neighborhood.GetCenterPixel();
  typename SpacingType::ComponentType space_i;
  typename SpacingType::ComponentType space_j;
  typename SpacingType::ComponentType spacing_i_Squared;

  for( unsigned int i = 0; i < ImageDimension; i++)
    {
    const ScalarValueType it_positionA
        = neighborhood.GetPixel( m_positionA[i] );
    const ScalarValueType it_positionB
        = neighborhood.GetPixel( m_positionB[i] );
    space_i = 0.5 / spacing[i];
    spacing_i_Squared = 1.0 / ( spacing[i] * spacing[i] );

    // First order partial derivatives
    gd->m_dx[i] = space_i * ( it_positionA - it_positionB );

    // Second order partial derivatives
    gd->m_dxy[i][i] = ( it_positionA + it_positionB - 2.0 * center_value )
                      * spacing_i_Squared;

    for( unsigned int j = i+1; j < ImageDimension; j++ )
      {
      space_j = 0.5 / spacing[j];
      gd->m_dxy[i][j] = gd->m_dxy[j][i]
            = ( neighborhood.GetPixel( m_positionAa[i][j] )
                - neighborhood.GetPixel( m_positionBa[i][j] )
                - neighborhood.GetPixel( m_positionCa[i][j] )
                + neighborhood.GetPixel( m_positionDa[i][j] ) )
              * ( space_i * space_j );
      }
    }
}

template< class TImageType >
typename AnisotropicDiffusionTensorFunction< TImageType >::PixelType
AnisotropicDiffusionTensorFunction< TImageType >
::ComputeFinalUpdateTerm(
    const DiffusionTensorNeighborhoodType &tensorNeighborhood,
    const GlobalDataStruct *gd) const
{
  DiffusionTensorType center_Tensor_value = tensorNeighborhood.GetCenterPixel();

  ScalarValueType pdWrtDiffusion1 = gd->m_DT_dxy[0][0] * gd->m_dx[0]
                                    + gd->m_DT_dxy[0][1] * gd->m_dx[1]
                                    + gd->m_DT_dxy[0][2] * gd->m_dx[2];

  ScalarValueType pdWrtDiffusion2 = gd->m_DT_dxy[1][0] * gd->m_dx[0]
                                    + gd->m_DT_dxy[1][1] * gd->m_dx[1]
                                    + gd->m_DT_dxy[1][2] * gd->m_dx[2];

  ScalarValueType pdWrtDiffusion3 = gd->m_DT_dxy[2][0] * gd->m_dx[0]
                                    + gd->m_DT_dxy[2][1] * gd->m_dx[1]
                                    + gd->m_DT_dxy[2][2] * gd->m_dx[2];

  ScalarValueType pdWrtImageIntensity1
      = center_Tensor_value(0,0) * gd->m_dxy[0][0]
        + center_Tensor_value(0,1) * gd->m_dxy[0][1]
        + center_Tensor_value(0,2) * gd->m_dxy[0][2];

  ScalarValueType pdWrtImageIntensity2
      = center_Tensor_value(1,0) * gd->m_dxy[1][0]
        + center_Tensor_value(1,1) * gd->m_dxy[1][1]
        + center_Tensor_value(1,2) * gd->m_dxy[1][2];

  ScalarValueType pdWrtImageIntensity3
      = center_Tensor_value(2,0) * gd->m_dxy[2][0]
        + center_Tensor_value(2,1) * gd->m_dxy[2][1]
        + center_Tensor_value(2,2) * gd->m_dxy[2][2];

  ScalarValueType total
      = pdWrtDiffusion1 + pdWrtDiffusion2 + pdWrtDiffusion3
        + pdWrtImageIntensity1 + pdWrtImageIntensity2 + pdWrtImageIntensity3;

  return ( PixelType ) ( total );
}

template < class TImageType >
template < class TPixel, unsigned int VImageDimension >
void
AnisotropicDiffusionTensorFunction<TImageType>
::CheckTimeStepStability(
    const itk::Image< TPixel, VImageDimension > * input,
    bool useImageSpacing )
{
  double minSpacing;
  if( useImageSpacing )
    {
    minSpacing = input->GetSpacing()[0];
    for( unsigned int i = 1; i < ImageDimension; i++ )
      {
      if( input->GetSpacing()[i] < minSpacing)
        {
        minSpacing = input->GetSpacing()[i];
        }
      }
    }
  else
    {
    minSpacing = 1.0;
    }

  // plus 1?
  double ratio
      = minSpacing / vcl_pow(2.0, static_cast<double>(ImageDimension) + 1);

  if( m_TimeStep > ratio )
    {
    itkWarningMacro(<< std::endl
      << "Anisotropic diffusion unstable time step:"
      << m_TimeStep << std::endl << "Minimum stable time step"
      << "for this image is " << ratio );
    }
}

} // end namespace itk

#endif
