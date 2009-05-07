#ifndef __itkRigidRegulizerMetric_h
#define __itkRigidRegulizerMetric_h

#include "itkSingleValuedCostFunction.h"

#include "itkBSplineDeformableTransform.h"
#include "itkRigidRegularizationDerivativeImageFilter.h"


namespace itk
{
  /** 
   * \class RigidRegulizerMetric
   * \brief A metric that calculates a rigid penalty term.
   *
   * 
   * \ingroup Metrics
   */

	template< unsigned int Dimension, class TScalarType >
  class RigidRegulizerMetric : public SingleValuedCostFunction
  {
  public:

		/** Standard itk stuff. */
    typedef RigidRegulizerMetric						Self;
    typedef SingleValuedCostFunction        Superclass;
    typedef SmartPointer<Self>              Pointer;
    typedef SmartPointer<const Self>        ConstPointer;

    itkNewMacro( Self );
    itkTypeMacro( RigidRegulizerMetric, SingleValuedCostFunction );

		/** Typedef's inherited from the superclass. */
    typedef typename Superclass::MeasureType         MeasureType;
    typedef typename Superclass::DerivativeType      DerivativeType;
    typedef typename Superclass::ParametersType      ParametersType;

   // typedef Superclass::Pointer             SingleValuedCostFunctionPointer;
		typedef TScalarType		ScalarType;
		/** Define the dimension. */
		itkStaticConstMacro( ImageDimension, unsigned int, Dimension );

		/** Typedef's for BSpline transform. */
		typedef BSplineDeformableTransform< ScalarType, Dimension, 3 >	BSplineTransformType;
		typedef typename BSplineTransformType::Pointer			BSplineTransformPointer;

		/** Typedef's for the coefficient image (which is a scalar image),
		 * for the coefficient vector image (which is a vector image,
		 * containing all components of the coefficient image), and
		 * for the rigidity image (which contains the rigidity coefficients).
		 */
		typedef typename BSplineTransformType::PixelType			CoefficientPixelType;
		typedef typename BSplineTransformType::ImageType			CoefficientImageType;
		typedef typename CoefficientImageType::Pointer				CoefficientImagePointer;
		typedef Vector< CoefficientPixelType,
			itkGetStaticConstMacro( ImageDimension ) >					CoefficientVectorType;
		typedef Image< CoefficientVectorType,
			itkGetStaticConstMacro( ImageDimension ) >					CoefficientVectorImageType;
		typedef typename CoefficientVectorImageType::Pointer	CoefficientVectorImagePointer;
		typedef CoefficientImageType													RigidityImageType;
		typedef CoefficientImagePointer												RigidityImagePointer;

		/** Typedef's for the rigid derivative filter. */
		typedef RigidRegularizationDerivativeImageFilter<
			CoefficientVectorImageType,
			CoefficientVectorImageType >												RigidDerivativeFilterType;
		typedef typename RigidDerivativeFilterType::Pointer		RigidDerivativeFilterPointer;

    /** The GetValue()-method returns the rigid penalty number. */
    virtual MeasureType GetValue(
			const ParametersType & parameters ) const;

    /** The GetDerivative()-method returns the rigid penalty derivative. */
    virtual void GetDerivative(
			const ParametersType & parameters,
			DerivativeType & derivative ) const;

    /** Same procedure as in GetValue and GetDerivative */
    virtual void GetValueAndDerivative(
      const ParametersType & parameters,
      MeasureType & value,
      DerivativeType & derivative ) const;

    /** Get the number of parameters. */
    virtual unsigned int GetNumberOfParameters(void) const;

		/** Set the BSpline transform in this class.
		 * This class expects a BSplineTransform! It is not suited for others.
		 */
		itkSetMacro( BSplineTransform, BSplineTransformPointer );

		/** Set the RigidityImage in this class. */
		itkSetMacro( RigidityImage, RigidityImagePointer );

		/** Set macro for using the spacing. */
		itkSetMacro( UseImageSpacing, bool );

		/** Set macro for the weight of the second order term. */
		itkSetMacro( SecondOrderWeight, ScalarType );

		/** Set the OutputDirectoryName. */
		itkSetStringMacro( OutputDirectoryName );

  protected:

    RigidRegulizerMetric();
    virtual ~RigidRegulizerMetric() {};

    void PrintSelf( std::ostream& os, Indent indent ) const{};

  private:

    RigidRegulizerMetric( const Self& );	// purposely not implemented
    void operator=( const Self& );				// purposely not implemented

		/** Member variables. */
		BSplineTransformPointer		m_BSplineTransform;
		RigidityImagePointer			m_RigidityImage;

		bool				m_UseImageSpacing;
		ScalarType	m_SecondOrderWeight;

		/** Name of the output directory. */
		std::string m_OutputDirectoryName;

  }; // end class RigidRegulizerMetric
  

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRigidRegulizerMetric.txx"
#endif

#endif // #ifndef __itkRigidRegulizerMetric_h
