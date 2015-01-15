/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/
#ifndef __itkRecursiveBSplineTransform_h
#define __itkRecursiveBSplineTransform_h

#include "itkAdvancedBSplineDeformableTransform.h"

#include "itkRecursiveBSplineInterpolationWeightFunction.h"
#include "itkRecursiveBSplineTransformImplementation.h"


namespace itk
{
/** \class RecursiveBSplineTransform
 * \brief A recursive implementation of the B-spline transform
 *
 * The class is templated coordinate representation type (float or double),
 * the space dimension and the spline order.
 *
 * \ingroup ITKTransform
 */

template< typename TScalarType = double,
  unsigned int NDimensions = 3,
  unsigned int VSplineOrder = 3 >
class RecursiveBSplineTransform :
  public AdvancedBSplineDeformableTransform< TScalarType, NDimensions, VSplineOrder >
{
public:
  /** Standard class typedefs. */
  typedef RecursiveBSplineTransform              Self;
    typedef AdvancedBSplineDeformableTransform<
      TScalarType, NDimensions, VSplineOrder >   Superclass;
  typedef SmartPointer<Self>                     Pointer;
  typedef SmartPointer<const Self>               ConstPointer;

  /** New macro for creation of through the object factory. */
  itkNewMacro( Self );

  /** Run-time type information (and related methods). */
  itkTypeMacro( RecursiveBSplineTransform, AdvancedBSplineDeformableTransform );

  /** Dimension of the domain space. */
  itkStaticConstMacro( SpaceDimension, unsigned int, NDimensions );

  /** The BSpline order. */
  itkStaticConstMacro( SplineOrder, unsigned int, VSplineOrder );

  /** Standard scalar type for this class. */
  typedef typename Superclass::ScalarType             ScalarType;
  typedef typename Superclass::ParametersType         ParametersType;
  typedef typename Superclass::ParametersValueType    ParametersValueType;
  typedef typename Superclass::NumberOfParametersType NumberOfParametersType;
  typedef typename Superclass::JacobianType           JacobianType;
  typedef typename Superclass::InputVectorType        InputVectorType;
  typedef typename Superclass::OutputVectorType       OutputVectorType;
  typedef typename Superclass::InputCovariantVectorType  InputCovariantVectorType;
  typedef typename Superclass::OutputCovariantVectorType OutputCovariantVectorType;
  typedef typename Superclass::InputVnlVectorType     InputVnlVectorType;
  typedef typename Superclass::OutputVnlVectorType    OutputVnlVectorType;
  typedef typename Superclass::InputPointType         InputPointType;
  typedef typename Superclass::OutputPointType        OutputPointType;

  /** Parameters as SpaceDimension number of images. */
  typedef typename Superclass::PixelType             PixelType;
  typedef typename Superclass::ImageType             ImageType;
  typedef typename Superclass::ImagePointer          ImagePointer;
  //typedef typename Superclass::CoefficientImageArray CoefficientImageArray;

  /** Typedefs for specifying the extend to the grid. */
  typedef typename Superclass::RegionType     RegionType;
  typedef typename Superclass::IndexType      IndexType;
  typedef typename Superclass::SizeType       SizeType;
  typedef typename Superclass::SpacingType    SpacingType;
  typedef typename Superclass::DirectionType  DirectionType;
  typedef typename Superclass::OriginType     OriginType;
  typedef typename Superclass::GridOffsetType GridOffsetType;
  typedef typename GridOffsetType::OffsetValueType  OffsetValueType;

  typedef typename Superclass::NonZeroJacobianIndicesType     NonZeroJacobianIndicesType;
  typedef typename Superclass::SpatialJacobianType            SpatialJacobianType;
  typedef typename Superclass::JacobianOfSpatialJacobianType  JacobianOfSpatialJacobianType;
  typedef typename Superclass::SpatialHessianType             SpatialHessianType;
  typedef typename Superclass::JacobianOfSpatialHessianType   JacobianOfSpatialHessianType;
  typedef typename Superclass::InternalMatrixType             InternalMatrixType;

  /** Interpolation weights function type. */
  typedef typename Superclass::WeightsFunctionType                WeightsFunctionType;
  typedef typename Superclass::WeightsFunctionPointer             WeightsFunctionPointer;
  typedef typename Superclass::WeightsType                        WeightsType;
  typedef typename Superclass::ContinuousIndexType                ContinuousIndexType;
  typedef typename Superclass::DerivativeWeightsFunctionType      DerivativeWeightsFunctionType;
  typedef typename Superclass::DerivativeWeightsFunctionPointer   DerivativeWeightsFunctionPointer;
  typedef typename Superclass::SODerivativeWeightsFunctionType    SODerivativeWeightsFunctionType;
  typedef typename Superclass::SODerivativeWeightsFunctionPointer SODerivativeWeightsFunctionPointer;

  /** Parameter index array type. */
  typedef typename Superclass::ParameterIndexArrayType ParameterIndexArrayType;

  typedef itk::RecursiveBSplineInterpolationWeightFunction<
    TScalarType, NDimensions, VSplineOrder >                      RecursiveBSplineWeightFunctionType;

  /** Compute point transformation. This one is commonly used.
   * It calls RecursiveBSplineTransformImplementation2::InterpolateTransformPoint
   * for a recursive implementation.
   */
  virtual OutputPointType TransformPoint( const InputPointType & point ) const;

  /** Compute point transformation. Calls the five-argument version, which uses
   * RecursiveBSplineTransformImplementation::InterpolateTransformPoint
   * for a less smart recursive implementation.
   */
  virtual OutputPointType TransformPointOld( const InputPointType & point ) const;
  virtual void TransformPoint( const InputPointType & inputPoint, OutputPointType & outputPoint,
    WeightsType & weights, ParameterIndexArrayType & indices, bool & inside ) const;

  /** Compute the Jacobian of the transformation. */
  virtual void GetJacobian(
    const InputPointType & ipp,
    JacobianType & j,
    NonZeroJacobianIndicesType & nzji ) const;

  /** Compute the spatial Jacobian of the transformation. */
  virtual void GetSpatialJacobian(
    const InputPointType & ipp,
    SpatialJacobianType & sj ) const;

protected:

  RecursiveBSplineTransform();
  virtual ~RecursiveBSplineTransform(){};

  typedef typename Superclass::JacobianImageType JacobianImageType;
  typedef typename Superclass::JacobianPixelType JacobianPixelType;

  typename RecursiveBSplineWeightFunctionType::Pointer m_RecursiveBSplineWeightFunction;

  virtual void ComputeNonZeroJacobianIndices(
    NonZeroJacobianIndicesType & nonZeroJacobianIndices,
    const RegionType & supportRegion ) const;

private:

  RecursiveBSplineTransform( const Self & ); // purposely not implemented
  void operator=( const Self & );            // purposely not implemented

}; // end class RecursiveBSplineTransform

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRecursiveBSplineTransform.hxx"
#endif

#endif /* __itkRecursiveBSplineTransform_h */
