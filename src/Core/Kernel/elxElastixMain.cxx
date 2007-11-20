#ifndef __elxElastixMain_cxx
#define __elxElastixMain_cxx


/** If running on a Windows-system, include "windows.h".
 *  This is to set the priority, but which does not work on cygwin */

#if defined(_WIN32) && !defined(__CYGWIN__)
  #include <windows.h>
#endif

#include "elxElastixMain.h"
#include "elxMacro.h"
#include "itkMultiThreader.h"



namespace elastix
{
	using namespace xl;

	/**
	 * ******************* Global variables *************************
	 * 
	 * Some global variables (not part of the ElastixMain class, used
	 * by xoutSetup.	 
	 */
	
	/** \todo move to ElastixMain class, as static vars? */

	/** xout TargetCells. */
	xoutbase_type		g_xout;
	xoutsimple_type g_WarningXout;
	xoutsimple_type g_ErrorXout;
	xoutsimple_type g_StandardXout;
	xoutsimple_type g_CoutOnlyXout;
	xoutsimple_type g_LogOnlyXout;
	std::ofstream		g_LogFileStream;

	/**
	 * ********************* xoutSetup ******************************
	 * 
	 * NB: this function is a global function, not part of the ElastixMain
	 * class!!
	 */

	int xoutSetup(const char * logfilename)
	{
		/** the namespace of xout: */
		using namespace xl;

		int returndummy = 0;
		
		set_xout(&g_xout);

		/** Open the logfile for writing. */
		g_LogFileStream.open( logfilename );
		if ( !g_LogFileStream.is_open() )
		{
			std::cerr << "ERROR: LogFile cannot be opened!" << std::endl;
			return 1;
		}

		/** Set std::cout and the logfile as outputs of xout. */
		returndummy |= xout.AddOutput("log", &g_LogFileStream);
		returndummy |= xout.AddOutput("cout", &std::cout);

		/** Set outputs of LogOnly and CoutOnly.*/
		returndummy |= g_LogOnlyXout.AddOutput( "log", &g_LogFileStream );
		returndummy |= g_CoutOnlyXout.AddOutput( "cout", &std::cout );

		/** Copy the outputs to the warning-, error- and standard-xouts. */
		g_WarningXout.SetOutputs( xout.GetCOutputs() );
		g_ErrorXout.SetOutputs( xout.GetCOutputs() );
		g_StandardXout.SetOutputs( xout.GetCOutputs() );

		g_WarningXout.SetOutputs( xout.GetXOutputs() );
		g_ErrorXout.SetOutputs( xout.GetXOutputs() );
		g_StandardXout.SetOutputs( xout.GetXOutputs() );

		/** Link the warning-, error- and standard-xouts to xout. */	
		returndummy |= xout.AddTargetCell( "warning", &g_WarningXout );
		returndummy |= xout.AddTargetCell( "error", &g_ErrorXout );
		returndummy |= xout.AddTargetCell( "standard", &g_StandardXout );
		returndummy |= xout.AddTargetCell( "logonly", &g_LogOnlyXout );
		returndummy |= xout.AddTargetCell( "coutonly", &g_CoutOnlyXout );

		/** Format the output. */
		xout["standard"] << std::fixed;
		xout["standard"] << std::showpoint;
		
		/** Return a value. */
		return returndummy;

	} // end xoutSetup


	/**
	 * ********************* Constructor ****************************
	 */

	ElastixMain::ElastixMain()
	{
		/** Initialize the components. */
		this->m_Configuration = ConfigurationType::New();

		this->m_Elastix = 0;
		
		this->m_FixedImagePixelType = "";
		this->m_FixedImageDimension = 0;

		this->m_MovingImagePixelType = "";
		this->m_MovingImageDimension = 0;

		this->m_DBIndex = 0;

		this->m_FixedImageContainer = 0;
		this->m_MovingImageContainer = 0;
	
		this->m_FinalTransform = 0;
		this->m_InitialTransform = 0;

	} // end Constructor


	/**
	 * ****************** Initialization of static members *********
	 */
	ElastixMain::ComponentDatabasePointer ElastixMain::s_CDB = 0;
	ElastixMain::ComponentLoaderPointer ElastixMain::s_ComponentLoader = 0;

	/**
	 * ********************** Destructor ****************************
	 */

	ElastixMain::~ElastixMain()
	{
	 //nothing
	} // end Destructor


	/**
	 * *************** EnterCommandLineParameters *******************
	 */

	void ElastixMain
		::EnterCommandLineArguments( ArgumentMapType & argmap )
	{
		/** Initialize the configuration object with the 
		 * command line parameters entered by the user.
		 */		
		int dummy = this->m_Configuration->Initialize( argmap );
		if ( dummy )
		{
			xout["error"] << "ERROR: Something went wrong during initialisation of the configuration object." << std::endl;
		}

	} // end EnterCommandLineParameters


	/**
	 * **************************** Run *****************************
	 *
	 * Assuming EnterCommandLineParameters has already been invoked.
	 * or that m_Configuration is initialised in another way.
	 */

	int ElastixMain::Run(void)
	{
    /** Set process properties. */
    this->SetProcessPriority();
    this->SetMaximumNumberOfThreads();

		/** Initialize database. */		
		int errorCode = this->InitDBIndex();
		if ( errorCode != 0 )
		{
			return errorCode;
		}

    /** Create the Elastix component */
    try 
    {
      /** Key "Elastix", see elxComponentLoader::InstallSupportedImageTypes(). */
      this->m_Elastix = this->CreateComponent( "Elastix" );
    }
		catch( itk::ExceptionObject & excp )
		{
			/** We just print the exception and let the programm quit. */
			xl::xout["error"] << excp << std::endl;
			errorCode = 1;
      return errorCode;
		}

    /** Set some information in the ElastixBase */
		this->GetElastixBase()->SetConfiguration( this->m_Configuration );
		this->GetElastixBase()->SetComponentDatabase(this->s_CDB);
		this->GetElastixBase()->SetDBIndex( this->m_DBIndex );

    /** Populate the component containers */
    this->GetElastixBase()->SetRegistrationContainer(
      this->CreateComponents( "Registration", "MultiResolutionRegistration", errorCode) );

    this->GetElastixBase()->SetFixedImagePyramidContainer(
      this->CreateComponents( "FixedImagePyramid", "FixedRecursiveImagePyramid", errorCode) );

    this->GetElastixBase()->SetMovingImagePyramidContainer( 
      this->CreateComponents( "MovingImagePyramid", "MovingRecursiveImagePyramid", errorCode) );
      
    this->GetElastixBase()->SetInterpolatorContainer(
      this->CreateComponents( "Interpolator", "BSplineInterpolator", errorCode) );

    this->GetElastixBase()->SetMetricContainer(
      this->CreateComponents( "Metric", "MattesMutualInformation", errorCode) );

    this->GetElastixBase()->SetOptimizerContainer(    
      this->CreateComponents( "Optimizer", "RegularStepGradientDescent", errorCode) );

    this->GetElastixBase()->SetResampleInterpolatorContainer(
      this->CreateComponents( "ResampleInterpolator", "FinalBSplineInterpolator", errorCode) );
      
    this->GetElastixBase()->SetResamplerContainer(
      this->CreateComponents( "Resampler", "DefaultResampler", errorCode) );
      
    this->GetElastixBase()->SetTransformContainer(
      this->CreateComponents( "Transform", "TranslationTransform", errorCode) );
      
    /** Check if all component could be created. */
		if ( errorCode != 0 )
		{
			xout["error"] << "ERROR:" << std::endl;
			xout["error"] << "One or more components could not be created." << std::endl;
			return 1;
		}
		
		/** Set the images and masks. If not set by the user, it is not a problem.
		 * ElastixTemplate will try to load them from disk. */
		this->GetElastixBase()->SetFixedImageContainer( this->GetFixedImageContainer() );
		this->GetElastixBase()->SetMovingImageContainer( this->GetMovingImageContainer() );
    this->GetElastixBase()->SetFixedMaskContainer( this->GetFixedMaskContainer() );
		this->GetElastixBase()->SetMovingMaskContainer( this->GetMovingMaskContainer() );

    /** Set the initial transform, if it happens to be there */
		this->GetElastixBase()->SetInitialTransform( this->GetInitialTransform() );
    
		/** Run elastix! */
		try
		{
			errorCode = this->GetElastixBase()->Run();
		}
		catch( itk::ExceptionObject & excp )
		{
			/** We just print the exception and let the programm quit. */
			xl::xout["error"] << excp << std::endl;
			errorCode = 1;
		}

    /** Return the final transform */
    this->m_FinalTransform = this->GetElastixBase()->GetFinalTransform();

		/** Store the images in ElastixMain. */
		this->SetFixedImageContainer(  this->GetElastixBase()->GetFixedImageContainer() );
		this->SetMovingImageContainer( this->GetElastixBase()->GetMovingImageContainer() );
    this->SetFixedMaskContainer(  this->GetElastixBase()->GetFixedMaskContainer() );
		this->SetMovingMaskContainer( this->GetElastixBase()->GetMovingMaskContainer() );
		
		/** Return a value. */
		return errorCode;

	} // end Run


	/**
	 * **************************** Run *****************************
	 *
	 * Calls EnterCommandLineParameters and then Run().
	 */

	int ElastixMain::Run( ArgumentMapType & argmap )
	{

		this->EnterCommandLineArguments( argmap );
		return this->Run();

	} // end Run

	/**
	 * ************************** InitDBIndex ***********************
	 *
	 * Checks if the configuration object has been initialised,
	 * determines the requested ImageTypes, and sets the m_DBIndex
	 * to the corresponding value (by asking the elx::ComponentDatabase).
	 */

	int ElastixMain::InitDBIndex(void)
	{
		/** Only do something when the configuration object wasn't initialized yet.*/
		if ( this->m_Configuration->Initialized() )
		{			
			/** FixedImagePixelType */
			if ( this->m_FixedImagePixelType.empty() )
			{
				/** Try to read it from the parameterfile. */
        this->m_FixedImagePixelType = "float";
				this->m_Configuration->ReadParameter( this->m_FixedImagePixelType,	"FixedInternalImagePixelType", 0 );
      }

			/** MovingImagePixelType */
			if ( this->m_MovingImagePixelType.empty() )
			{
				/** Try to read it from the parameterfile. */
        this->m_MovingImagePixelType = "float";
				this->m_Configuration->ReadParameter( this->m_MovingImagePixelType, "MovingInternalImagePixelType", 0 );
			}

			/** FixedImageDimension */
			if ( this->m_FixedImageDimension == 0 )
			{
				/** Try to read it from the parameterfile. */
				this->m_Configuration->ReadParameter( this->m_FixedImageDimension, "FixedImageDimension", 0 );

				if ( this->m_FixedImageDimension == 0 )
				{
					xout["error"] << "ERROR:" << std::endl;
					xout["error"] << "The FixedImageDimension is not given." << std::endl;
					return 1;
				}
			}

			/** MovingImageDimension */
			if ( this->m_MovingImageDimension == 0 )
			{
				/** Try to read it from the parameterfile. */
				this->m_Configuration->ReadParameter( this->m_MovingImageDimension, "MovingImageDimension", 0 );

				if ( this->m_MovingImageDimension == 0 )
				{
					xout["error"] << "ERROR:" << std::endl;
					xout["error"] << "The MovingImageDimension is not given." << std::endl;
					return 1;
				}
			}
			
			/** Load the components. */
			if (this->s_CDB.IsNull())
			{
				int loadReturnCode = this->LoadComponents();
				if (loadReturnCode !=0)
				{
					xout["error"] << "Loading components failed" << std::endl;
					return loadReturnCode;
				}
			}

			if (this->s_CDB.IsNotNull())
			{
				/** Get the DBIndex from the ComponentDatabase. */
				this->m_DBIndex = this->s_CDB->GetIndex(
					this->m_FixedImagePixelType,
					this->m_FixedImageDimension,			
					this->m_MovingImagePixelType,
					this->m_MovingImageDimension );
				if ( this->m_DBIndex == 0 )
				{
					xout["error"] << "ERROR:" << std::endl;
					xout["error"] << "Something went wrong in the ComponentDatabase" << std::endl;
					return 1;
				}
			} //end if s_CDB!=0
	
		} // end if m_Configuration->Initialized();
		else
		{
			xout["error"] << "ERROR:" << std::endl;
			xout["error"] << "The configuration object has not been initialised." << std::endl;
			return 1;
		}

		/** Return an OK value. */
		return 0;

	} // end InitDBIndex


	/**
	 * ********************* SetElastixLevel ************************
	 */

	void ElastixMain::SetElastixLevel( unsigned int level )
	{
		/** Call SetElastixLevel from MyConfiguration. */
		this->m_Configuration->SetElastixLevel( level );

	} // end SetElastixLevel


	/**
	 * ********************* GetElastixLevel ************************
	 */

	unsigned int ElastixMain::GetElastixLevel(void)
	{
		/** Call GetElastixLevel from MyConfiguration. */
		return this->m_Configuration->GetElastixLevel();

	} // end GetElastixLevel


	/**
	 * ********************* LoadComponents **************************
	 *
	 * Look for dlls, load them, and call the install function.
	 */
	
	int ElastixMain::LoadComponents(void)
	{
		/** Create a ComponentDatabase. */
		if ( this->s_CDB.IsNull() )
		{
			this->s_CDB = ComponentDatabaseType::New();
		}

		/** Create a ComponentLoader and set the database. */
		if ( this->s_ComponentLoader.IsNull() )
		{
			this->s_ComponentLoader = ComponentLoaderType::New();
			this->s_ComponentLoader->SetComponentDatabase( s_CDB );
		}

		/** Get the current program. */
		const char * argv0 = this->m_Configuration->GetCommandLineArgument( "-argv0" );

		/** Load the components. */
		return this->s_ComponentLoader->LoadComponents( argv0 );
		
	} // end LoadComponents

		
	/**
	 * ********************* UnloadComponents **************************
	 */
	
	void ElastixMain::UnloadComponents(void)
	{
				
		s_CDB = 0;
		s_ComponentLoader->SetComponentDatabase( 0 );

		if (s_ComponentLoader)	
		{
			s_ComponentLoader->UnloadComponents();
		}
	} // end UnloadComponents


  /**
   * ************************* GetElastixBase ***************************
   */

  ElastixMain::ElastixBaseType * ElastixMain::GetElastixBase(void) const
  {
    ElastixBaseType * testpointer;
    /** Convert ElastixAsObject to a pointer to an ElastixBaseType. */
		testpointer = dynamic_cast<ElastixBaseType *>( this->m_Elastix.GetPointer() );
    if ( !testpointer )
    {
      itkExceptionMacro( << "Probably GetElastixBase() is called before having called Run()");
    }
    return testpointer;
  } // end GetElastixBase


  /**
   * ************************* CreateComponent ***************************
   */

  ElastixMain::ObjectPointer ElastixMain::CreateComponent( const ComponentDescriptionType & name )
  {
    /** A pointer to the ::New() function */
    PtrToCreator testcreator = 0;
    ObjectPointer testpointer = 0;
 		testcreator = this->s_CDB->GetCreator( name,	this->m_DBIndex );
		testpointer = testcreator ? testcreator() : NULL;
    if ( testpointer.IsNull() )
    {
      itkExceptionMacro( << "The following component could not be created: " << name );
    }
    return testpointer;

  } // end CreateComponent


  /** 
   * *********************** CreateComponents *****************************
   */

  ElastixMain::ObjectContainerPointer ElastixMain::CreateComponents(
    const std::string & key,
    const ComponentDescriptionType & defaultComponentName,
    int & errorcode )
  {
    ComponentDescriptionType componentName = defaultComponentName;
    unsigned int componentnr = 0;
    int returncode = 0;
    ObjectContainerPointer objectContainer = ObjectContainerType::New();
    objectContainer->Initialize();

    /** If the user hasn't specified any component names, use
     * the default, and give a warning */
    returncode = this->m_Configuration->ReadParameter(
      componentName, key.c_str(), componentnr, false );
    try
    {
      objectContainer->CreateElementAt( componentnr ) = 
        this->CreateComponent( componentName );
    }
    catch ( itk::ExceptionObject & excp )
    {
      xout["error"] 
        << "ERROR: error occured while creating " 
        << key << " "
        << componentnr  << "."  << std::endl;
      xout["error"] << excp << std::endl;
      errorcode = 1;
      return objectContainer;
    }
        
    /** Check if more than one component name is given.  */
    while ( returncode == 0 )
    {
      ++componentnr;
      returncode = this->m_Configuration->ReadParameter(
        componentName, key.c_str() , componentnr, true );
      if ( returncode == 0 )   
      {
        try
        {
          objectContainer->CreateElementAt( componentnr ) = 
            this->CreateComponent( componentName );
        }
        catch ( itk::ExceptionObject & excp )
        {
          xout["error"] 
            << "ERROR: error occured while creating " 
            << key << " "
            << componentnr  << "."  << std::endl;
          xout["error"] << excp << std::endl;
          errorcode = 1;
          return objectContainer;
        } 
      }  // end if
    } // end while

    return objectContainer;
  } // end CreateComponents


  /**
   * *********************** SetProcessPriority *************************
   */

  void ElastixMain::SetProcessPriority(void) const
  {
    /** If wanted, set the priority of this process high or below normal. */
		std::string processPriority = "";
		processPriority = this->m_Configuration->GetCommandLineArgument( "-priority" );
		if ( processPriority == "high" )
		{
      #if defined(_WIN32) && !defined(__CYGWIN__)
			SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );
			#endif
		}
		else if ( processPriority == "belownormal" )
		{
      #if defined(_WIN32) && !defined(__CYGWIN__)
			SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS );
			#endif
		}   

  } // end SetProcessPriority


  /**
   * *********************** SetMaximumNumberOfThreads *************************
   */

  void ElastixMain::SetMaximumNumberOfThreads(void) const
  {
    /** If wanted, set the priority of this process high or below normal. */
    std::string maximumNumberOfThreadsString = "";
		maximumNumberOfThreadsString = this->m_Configuration->GetCommandLineArgument( "-threads" );

    if ( maximumNumberOfThreadsString != "" )
    {
      const int maximumNumberOfThreads =
        atoi( maximumNumberOfThreadsString.c_str() );
      itk::MultiThreader::SetGlobalMaximumNumberOfThreads(
        maximumNumberOfThreads );
    }

  } // end SetMaximumNumberOfThreads
	
		
} // end namespace elastix

#endif // end #ifndef __elxElastixMain_cxx

