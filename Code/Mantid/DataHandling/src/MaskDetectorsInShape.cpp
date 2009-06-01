//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
	namespace DataHandling
	{
		// Register the algorithm into the algorithm factory
		DECLARE_ALGORITHM(MaskDetectorsInShape)

		using namespace Kernel;
		using namespace API;
		using namespace DataObjects;

		// Initialise the logger
		Kernel::Logger& MaskDetectorsInShape::g_log = Kernel::Logger::get("MaskDetectorsInShape");

		/// (Empty) Constructor
		MaskDetectorsInShape::MaskDetectorsInShape() {}

		/// Destructor
		MaskDetectorsInShape::~MaskDetectorsInShape() {}

		void MaskDetectorsInShape::init()
		{
			declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","",Direction::InOut));
			declareProperty("ShapeXML","",new MandatoryValidator<std::string>());
			declareProperty("IncludeMonitors",false);
		}

		void MaskDetectorsInShape::exec()
		{
			// Get the input workspace
			Workspace2D_sptr WS = getProperty("Workspace");
			
			const bool includeMonitors = getProperty("IncludeMonitors");
			const std::string shapeXML = getProperty("ShapeXML");


			std::vector<int> foundDets = runFindDetectorsInShape(WS,shapeXML,includeMonitors);
      if ( foundDets.empty() )
      {
        g_log.information("No detectors were found in the shape, nothing was masked");
        return;
      }
			runMaskDetectors(WS,foundDets);
			setProperty("Workspace",WS);
		}

		/// Run the FindDetectorsInShape sub-algorithm
    std::vector<int> MaskDetectorsInShape::runFindDetectorsInShape(DataObjects::Workspace2D_sptr workspace,
			const std::string shapeXML, const bool includeMonitors)
    {
      IAlgorithm_sptr alg = createSubAlgorithm("FindDetectorsInShape",0,85);
			alg->setPropertyValue("IncludeMonitors", includeMonitors?"1":"0");
			alg->setPropertyValue("ShapeXML",shapeXML);
      alg->setProperty<MatrixWorkspace_sptr>("Workspace",workspace);
      try
      {
        if (!alg->execute())
				{
					throw std::runtime_error("FindDetectorsInShape sub-algorithm has not executed successfully\n");
				}
      }
      catch (std::runtime_error&)
      {
    	  g_log.error("Unable to successfully execute FindDetectorsInShape sub-algorithm");
				throw;
      }

			//extract the results
			return alg->getProperty("DetectorList");
    }

    void MaskDetectorsInShape::runMaskDetectors(DataObjects::Workspace2D_sptr workspace, const std::vector<int> detectorIds)
    {
      IAlgorithm_sptr alg = createSubAlgorithm("MaskDetectors",85,100);
			alg->setProperty<std::vector<int> >("DetectorList", detectorIds);
      alg->setProperty<Workspace2D_sptr>("Workspace",workspace);
      try
      {
        if (!alg->execute())
				{
					throw std::runtime_error("MaskDetectors sub-algorithm has not executed successfully\n");
				}
      }
      catch (std::runtime_error&)
      {
    	  g_log.error("Unable to successfully execute MaskDetectors sub-algorithm");
				throw;
      }
    }

	} // namespace DataHandling
} // namespace Mantid

