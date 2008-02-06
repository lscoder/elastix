#ifndef __elxSimilarityTransform_HXX_
#define __elxSimilarityTransform_HXX_

#include "elxSimilarityTransform.h"

namespace elastix
{
  using namespace itk;
  
  /**
   * ********************* Constructor ****************************
   */
  
  template <class TElastix>
    SimilarityTransformElastix<TElastix>
    ::SimilarityTransformElastix()
  {
    this->m_SimilarityTransform = SimilarityTransformType::New();
    this->SetCurrentTransform( this->m_SimilarityTransform );

  } // end Constructor()
  
  
  /**
   * ******************* BeforeRegistration ***********************
   */
  
  template <class TElastix>
    void SimilarityTransformElastix<TElastix>
    ::BeforeRegistration(void)
  {
    /** Task 1 - Set center of rotation and initial translation. */
    this->InitializeTransform();
            
    /** Task 2 - Set the scales. */
    this->SetScales();


  } // end BeforeRegistration()

  
  /**
   * ************************* ReadFromFile ************************
   */

  template <class TElastix>
  void SimilarityTransformElastix<TElastix>::
    ReadFromFile(void)
  {
    /** Variables. */
    InputPointType centerOfRotationPoint;
    centerOfRotationPoint.Fill( 0.0 );
    bool pointRead = false;
    bool indexRead = false;

    /** Try first to read the CenterOfRotationPoint from the 
     * transform parameter file, this is the new, and preferred
     * way, since elastix 3.402.
     */    
    pointRead = ReadCenterOfRotationPoint( centerOfRotationPoint );

    /** If this did not succeed, probably a transform parameter file
     * is trying to be read that was generated using an older elastix
     * version. Try to read it as an index, and convert to point.
     */
    if ( !pointRead )
    {
      indexRead = ReadCenterOfRotationIndex( centerOfRotationPoint );
    }

    if ( !pointRead && !indexRead )
    {
      xl::xout["error"] << "ERROR: No center of rotation is specified in the transform parameter file." << std::endl;
      itkExceptionMacro( << "Transform parameter file is corrupt." )
    }

    /** Set the center in this Transform. */
    this->m_SimilarityTransform->SetCenter( centerOfRotationPoint );

    /** Call the ReadFromFile from the TransformBase.
     * BE AWARE: Only call Superclass2::ReadFromFile() after CenterOfRotation
     * is set, because it is used in the SetParameters()-function of this transform.
     */
    this->Superclass2::ReadFromFile();

  } // end ReadFromFile()


  /**
   * ************************* WriteToFile ************************
   */
  
  template <class TElastix>
    void SimilarityTransformElastix<TElastix>
    ::WriteToFile( const ParametersType & param )
  {
    /** Call the WriteToFile from the TransformBase. */
    this->Superclass2::WriteToFile( param );

    /** Write SimilarityTransform specific things. */
    xout["transpar"] << std::endl << "// SimilarityTransform specific" << std::endl;

    /** Set the precision of cout to 10. */
    xout["transpar"] << std::setprecision( 10 );

    /** Get the center of rotation point and write it to file. */
    InputPointType rotationPoint = this->m_SimilarityTransform->GetCenter();
    xout["transpar"] << "(CenterOfRotationPoint ";
    for ( unsigned int i = 0; i < SpaceDimension - 1; i++ )
    {
      xout["transpar"] << rotationPoint[ i ] << " ";
    }
    xout["transpar"] << rotationPoint[ SpaceDimension - 1 ] << ")" << std::endl;

    /** Set the precision back to default value. */
    xout["transpar"] << std::setprecision( this->m_Elastix->GetDefaultOutputPrecision() );
  
  } // end WriteToFile()

  
  /**
   * ************************* InitializeTransform *********************
   */
  
  template <class TElastix>
    void SimilarityTransformElastix<TElastix>
    ::InitializeTransform( void )
  {
    /** Set the parameters to mimic the identity transform. */
    this->m_SimilarityTransform->SetIdentity();
    
    /** Try to read CenterOfRotationIndex from parameter file,
     * which is the rotationPoint, expressed in index-values.
     */
    IndexType centerOfRotationIndex;
    InputPointType centerOfRotationPoint;
    bool CORIndexInImage = true;
    bool CORPointInImage = true;
    bool centerGivenAsIndex = true;
    bool centerGivenAsPoint = true;
    SizeType fixedImageSize = this->m_Registration->GetAsITKBaseType()->
      GetFixedImage()->GetLargestPossibleRegion().GetSize();
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      /** Initilialize. */
      centerOfRotationIndex[ i ] = 0;
      centerOfRotationPoint[ i ] = 0.0;
      /** Check COR index: Returns zero when parameter was in the parameter file. */
      int returncodeI = this->m_Configuration->ReadParameter(
        centerOfRotationIndex[ i ], "CenterOfRotation", i, true );
      if ( returncodeI != 0 )
      {
        centerGivenAsIndex &= false;
      }
      /** Check COR point: Returns zero when parameter was in the parameter file. */
      int returncodeP = this->m_Configuration->ReadParameter(
        centerOfRotationPoint[ i ], "CenterOfRotationPoint", i, true );
      if ( returncodeP != 0 )
      {
        centerGivenAsPoint &= false;
      }
      /** Check if centerOfRotationIndex is within image. */
      if ( centerOfRotationIndex[ i ] < 0 ||
        centerOfRotationIndex[ i ] > fixedImageSize[ i ] )
      {
        CORIndexInImage = false;
      }
    } // end loop over SpaceDimension

    /** Check if centerOfRotationPoint is within image. */
    IndexType indexOfPoint;
    this->m_Registration->GetAsITKBaseType()->GetFixedImage()->
      TransformPhysicalPointToIndex( centerOfRotationPoint, indexOfPoint );
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      if ( centerOfRotationPoint[ i ] < 0 ||
        centerOfRotationPoint[ i ] > fixedImageSize[ i ] )
      {
        CORPointInImage = false;
      }
    }
    
    /** Give a warning if necessary. */
    if ( !CORIndexInImage && centerGivenAsIndex )
    {
      xl::xout["warning"] << "WARNING: Center of Rotation (index) is not within image boundaries!" << std::endl;
    }

    /** Give a warning if necessary. */
    if ( !CORPointInImage && centerGivenAsPoint && !centerGivenAsIndex )
    {
      xl::xout["warning"] << "WARNING: Center of Rotation (point) is not within image boundaries!" << std::endl;
    }

    /** Check if user wants automatic transform initialization; false by default. */
    std::string automaticTransformInitializationString( "false" );
    bool automaticTransformInitialization = false;
    this->m_Configuration->ReadParameter(
      automaticTransformInitializationString,
      "AutomaticTransformInitialization", 0 );
    if ( automaticTransformInitializationString == "true" &&
      this->Superclass1::GetInitialTransform() == 0 )
    {
      automaticTransformInitialization = true;
    }

    /** 
     * Run the itkTransformInitializer if:
     * - No center of rotation was given, or
     * - The user asked for AutomaticTransformInitialization
     */
    bool centerGiven = centerGivenAsIndex || centerGivenAsPoint;
    if ( !centerGiven || automaticTransformInitialization ) 
    {
     
      /** Use the TransformInitializer to determine a center of 
      * of rotation and an initial translation */
      TransformInitializerPointer transformInitializer = 
        TransformInitializerType::New();
      transformInitializer->SetFixedImage(
        this->m_Registration->GetAsITKBaseType()->GetFixedImage() );
      transformInitializer->SetMovingImage(
        this->m_Registration->GetAsITKBaseType()->GetMovingImage() );
      transformInitializer->SetTransform( this->m_SimilarityTransform );
      transformInitializer->GeometryOn();
      transformInitializer->InitializeTransform();
    }

    /** Set the translation to zero, if no AutomaticTransformInitialization
     * was desired.
     */
    if ( !automaticTransformInitialization )
    {
      OutputVectorType noTranslation;
      noTranslation.Fill(0.0);
      this->m_SimilarityTransform->SetTranslation( noTranslation );
    }

    /** Set the center of rotation if it was entered by the user. */
    if ( centerGiven )
    {
      if ( centerGivenAsIndex )
      {
        /** Convert from index-value to physical-point-value.*/
        this->m_Registration->GetAsITKBaseType()->GetFixedImage()->
          TransformIndexToPhysicalPoint( centerOfRotationIndex, centerOfRotationPoint );
      }
      this->m_SimilarityTransform->SetCenter( centerOfRotationPoint );
    }

    /** Apply the initial transform to the center of rotation, if 
     * composition is used to combine the initial transform with the
     * the current (euler) transform. */
    if ( this->GetUseComposition() && this->Superclass1::GetInitialTransform() != 0 )
    {
      InputPointType transformedCenterOfRotationPoint = 
        this->Superclass1::GetInitialTransform()->TransformPoint( 
        this->m_SimilarityTransform->GetCenter() );
      this->m_SimilarityTransform->SetCenter(
        transformedCenterOfRotationPoint );
    }

    /** Set the initial parameters in this->m_Registration. */
    this->m_Registration->GetAsITKBaseType()->
      SetInitialTransformParameters( this->GetParameters() );

  } // end InitializeTransform()

  
  /**
   * ************************* SetScales *********************
   */

  template <class TElastix>
    void SimilarityTransformElastix<TElastix>
    ::SetScales( void )
  {
    /** Create the new scales. */
    const unsigned int N = this->GetNumberOfParameters();
    ScalesType newscales( N );
    newscales.Fill( 1.0 );

    /** Check if automatic scales estimation is desired */
    bool automaticScalesEstimation = false;
    this->m_Configuration->ReadParameter( automaticScalesEstimation,
      "AutomaticScalesEstimation", 0 );

    if ( automaticScalesEstimation )
    {
      elxout << "Scales are estimated automatically." << std::endl;
      this->AutomaticScalesEstimation( newscales );
    }
    else
    {
      /** If the dimension is 2, then the first parameter represents the
      * isotropic scaling, the the second the rotation, and the third and
      * fourth the translation.
      * If the dimension is 3, then the first three represent rotations,
      * the second three translations, and the last parameter the isotropic
      * scaling.
      */

      /** Create the scales and set to default values. */    
      if ( SpaceDimension == 2 )
      {
        newscales[ 0 ] = 10000.0;
        newscales[ 1 ] = 100000.0;
      }
      else if ( SpaceDimension == 3 )
      {
        newscales[ 6 ] = 10000.0;
        for ( unsigned int i = 0; i < 3; i++ ) newscales[ i ] = 100000.0;
      }

      // as it is now you should supply all scales.
      /** Get the scales from the parameter file. */
      for ( unsigned int i = 0; i < N; i++ )
      {
        this->GetConfiguration()->ReadParameter( newscales[ i ],
          "Scales", this->GetComponentLabel(), i, -1 );
      }      
    }  // end else: no automatic parameter estimation

    elxout << "Scales for transform parameters are: " << newscales << std::endl;

    /** Set the scales into the optimizer. */
    this->m_Registration->GetAsITKBaseType()->GetOptimizer()->SetScales( newscales );    

  } // end SetScales
  

  /**
   * ******************** ReadCenterOfRotationIndex *********************
   */

  template <class TElastix>
  bool SimilarityTransformElastix<TElastix>::
    ReadCenterOfRotationIndex( InputPointType & rotationPoint )
  {
    /** Try to read CenterOfRotationIndex from the transform parameter
     * file, which is the rotationPoint, expressed in index-values.
     */
    IndexType centerOfRotationIndex;
    bool centerGivenAsIndex = true;
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      centerOfRotationIndex[ i ] = 0;
      /** Returns zero when parameter was in the parameter file */
      int returncode = this->m_Configuration->ReadParameter(
        centerOfRotationIndex[ i ], "CenterOfRotation", i, true );
      if ( returncode != 0 )
      {
        centerGivenAsIndex &= false;
      }
    } // end for i
    if ( !centerGivenAsIndex )
    {
      return false;
    }
  
    /** Get spacing, origin and size of the fixed image.
     * We put this in a dummy image, so that we can correctly
     * calculate the center of rotation in world coordinates.
     */
    SpacingType   spacing;
    IndexType     index;
    PointType     origin;
    SizeType      size;
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      /** Read size from the parameter file. Zero by default, which is illegal. */
      size[ i ] = 0; 
      this->m_Configuration->ReadParameter( size[ i ], "Size", i );

      /** Default index. Read index from the parameter file. */
      index[ i ] = 0;
      this->m_Configuration->ReadParameter( index[ i ], "Index", i );

      /** Default spacing. Read spacing from the parameter file. */
      spacing[ i ] = 1.0;
      this->m_Configuration->ReadParameter( spacing[ i ], "Spacing", i );

      /** Default origin. Read origin from the parameter file. */
      origin[ i ] = 0.0;
      this->m_Configuration->ReadParameter( origin[ i ], "Origin", i );
    }

    /** Check for image size. */
    bool illegalSize = false;
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      if ( size[ i ] == 0 )
      {
        illegalSize = true;
      }
    }
    if ( illegalSize )
    {
      xl::xout["error"] << "ERROR: One or more image sizes are 0!" << std::endl;
      return false;
    }
    
    /** Make a temporary image with the right region info,
    * so that the TransformIndexToPhysicalPoint-functions will be right.
    */
    typedef FixedImageType DummyImageType; 
    typename DummyImageType::Pointer dummyImage = DummyImageType::New();
    RegionType region;
    region.SetIndex( index );
    region.SetSize( size );
    dummyImage->SetRegions( region );
    dummyImage->SetOrigin( origin );
    dummyImage->SetSpacing( spacing );

    /** Convert center of rotation from index-value to physical-point-value.*/
    dummyImage->TransformIndexToPhysicalPoint( 
      centerOfRotationIndex, rotationPoint );

    /** Succesfully read centerOfRotation as Index */
    return true;

  } //end ReadCenterOfRotationIndex()


  /**
   * ******************** ReadCenterOfRotationPoint *********************
   */

  template <class TElastix>
  bool SimilarityTransformElastix<TElastix>::
    ReadCenterOfRotationPoint(InputPointType & rotationPoint)
  {

    /** Try to read CenterOfRotationPoint from the transform parameter
     * file, which is the rotationPoint, expressed in world coordinates.
     */
    InputPointType centerOfRotationPoint;
    bool centerGivenAsPoint = true;
    for ( unsigned int i = 0; i < SpaceDimension; i++ )
    {
      centerOfRotationPoint[ i ] = 0;
      /** Returns zero when parameter was in the parameter file */
      int returncode = this->m_Configuration->ReadParameter(
        centerOfRotationPoint[ i ], "CenterOfRotationPoint", i, true );
      if ( returncode != 0 )
      {
        centerGivenAsPoint &= false;
      }
    } // end for i
    if ( !centerGivenAsPoint )
    {
      return false;
    }
  
    /** copy the temporary variable into the output of this function,
     * if everything went ok  */
    rotationPoint = centerOfRotationPoint;
    
    /** Succesfully read centerOfRotation as Point */
    return true;

  } //end ReadCenterOfRotationPoint()


} // end namespace elastix


#endif // end #ifndef __elxSimilarityTransform_HXX_

