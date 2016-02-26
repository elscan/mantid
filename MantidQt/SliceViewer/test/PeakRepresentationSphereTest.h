#ifndef SLICE_VIEWER_PEAK_REPRESENTATION_SPHERE_TEST_H_
#define SLICE_VIEWER_PEAK_REPRESENTATION_SPHERE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/PeakRepresentationSphere.h"
#include "MockObjects.h"

#if 0
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidMDAlgorithms/SaveMD2.h"
#endif

using namespace MantidQt::SliceViewer;
using namespace testing;

#if 0
namespace {
// Add A Fake 'Peak' to both the event data and to the peaks workspace
void addFakeEllipsoid(const Mantid::Kernel::V3D &peakHKL, const int &totalNPixels,
                      const int &nEvents, const double tofGap,
                      Mantid::DataObjects::EventWorkspace_sptr &eventWS,
                      Mantid::DataObjects::PeaksWorkspace_sptr &peaksWS) {
  // Create the peak and add it to the peaks ws
  Mantid::DataObjects::Peak *peak = peaksWS->createPeakHKL(peakHKL);
  peaksWS->addPeak(*peak);
  const int detectorId = peak->getDetectorID();
  const double tofExact = peak->getTOF();
  delete peak;

  Mantid::DataObjects::EventList &el = eventWS->getEventList(detectorId - totalNPixels);

  // Add more events to the event list corresponding to the peak centre
  double start = tofExact - (double(nEvents) / 2 * tofGap);
  for (int i = 0; i < nEvents; ++i) {
    const double tof = start + (i * tofGap);
    el.addEventQuickly(Mantid::DataObjects::TofEvent(tof));
  }
}

// Create diffraction data for test schenarios
boost::tuple<Mantid::DataObjects::EventWorkspace_sptr, Mantid::DataObjects::PeaksWorkspace_sptr>
createDiffractionData(const int nPixels = 100, const int nEventsPerPeak = 20,
                      const double tofGapBetweenEvents = 10) {
  Mantid::Geometry::Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(
          1 /*num_banks*/, nPixels /*pixels in each direction yields n by n*/,
          0.01, 1.0);

  // Create a peaks workspace
  auto peaksWS = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
  // Set the instrument to be the fake rectangular bank above.
  peaksWS->setInstrument(inst);
  // Set the oriented lattice for a cubic crystal
  Mantid::Geometry::OrientedLattice ol(6, 6, 6, 90, 90, 90);
  ol.setUFromVectors(Mantid::Kernel::V3D(6, 0, 0), Mantid::Kernel::V3D(0, 6, 0));
  peaksWS->mutableSample().setOrientedLattice(&ol);

  // Make an event workspace and add fake peak data
  auto eventWS = boost::make_shared<Mantid::DataObjects::EventWorkspace>();
  eventWS->setInstrument(inst);
  eventWS->initialize(nPixels * nPixels /*n spectra*/, 3 /* x-size */,
                      3 /* y-size */);
  eventWS->getAxis(0)->setUnit("TOF");
  // Give the spectra-detector mapping for all event lists
  const int nPixelsTotal = nPixels * nPixels;
  for (int i = 0; i < nPixelsTotal; ++i) {
    auto &el = eventWS->getOrAddEventList(i);
    el.setDetectorID(i + nPixelsTotal);
  }

  // Add some peaks which should correspond to real reflections (could
  // calculate these). Same function also adds a fake ellipsoid
  addFakeEllipsoid(Mantid::Kernel::V3D(1, -5, -3), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(Mantid::Kernel::V3D(1, -4, -4), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(Mantid::Kernel::V3D(1, -3, -5), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(Mantid::Kernel::V3D(1, -4, -2), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(Mantid::Kernel::V3D(1, -4, 0), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);
  addFakeEllipsoid(Mantid::Kernel::V3D(2, -3, -4), nPixelsTotal, nEventsPerPeak,
                   tofGapBetweenEvents, eventWS, peaksWS);

  // Return test data.
  return boost::tuple<Mantid::DataObjects::EventWorkspace_sptr, Mantid::DataObjects::PeaksWorkspace_sptr>(eventWS,
                                                                peaksWS);
}
}
#endif

class PeakRepresentationSphereExposeProtectedWrapper
    : public PeakRepresentationSphere
{
public:
    PeakRepresentationSphereExposeProtectedWrapper(
        const Mantid::Kernel::V3D &origin, const double &peakRadius,
        const double &backgroundInnerRadius,
        const double &backgroundOuterRadius)
        : PeakRepresentationSphere(origin, peakRadius, backgroundInnerRadius,
                                   backgroundOuterRadius)
    {
    }
    std::shared_ptr<PeakPrimitives>
    getDrawingInformationWrapper(
        PeakRepresentationViewInformation viewInformation)
    {
        return getDrawingInformation(viewInformation);
    }
};

class PeakRepresentationSphereTest : public CxxTest::TestSuite
{
public:
    void test_getRadius_gets_radius_if_background_is_not_shown()
    {
        // Arrange
        Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius = 3;
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act + Assert
        TSM_ASSERT(radius, peak.getEffectiveRadius());

    }

    void test_getRadius_gets_outer_background_radius_if_background_is_shown()
    {
        // Arrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius = 3;
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        peak.showBackgroundRadius(true);

        // Act + Assert
        TSM_ASSERT(outerBackgroundRadius, peak.getEffectiveRadius());
    }

    void test_handle_outer_background_radius_zero()
    {
        // Arrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius
            = 0; // This can happen using IntegratePeaksMD.
        PeakRepresentationSphereExposeProtectedWrapper peak(
            origin, radius, innerBackgroundRadius, outerBackgroundRadius);

        peak.showBackgroundRadius(true);

        const double slicePoint = innerBackgroundRadius;
        peak.setSlicePoint(slicePoint);

        // View Settings Scale 1:1 on both x and y for simplicity.
        PeakRepresentationViewInformation viewInformation;
        viewInformation.viewHeight = 1.0;
        viewInformation.viewWidth = 1.0;
        viewInformation.windowHeight = 1.0;
        viewInformation.windowWidth = 1.0;
        viewInformation.xOriginWindow = 1;
        viewInformation.yOriginWindow = 1;

        // Act
        auto drawingInformation
            = peak.getDrawingInformationWrapper(viewInformation);

        // Assert
        auto drawingInformationSphere
            = std::static_pointer_cast<PeakPrimitiveCircle>(
                drawingInformation);
        // The Return object should be initialized to zero in every field.
        TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusX,
                         drawingInformationSphere->backgroundInnerRadiusX);
        TS_ASSERT_EQUALS(drawingInformationSphere->backgroundOuterRadiusY,
                         drawingInformationSphere->backgroundInnerRadiusY);
    }

    void
    test_that_setting_slice_point_to_intersect_produces_valid_drawing_information()
    {
        // Arrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius = 3;
        PeakRepresentationSphereExposeProtectedWrapper peak(
            origin, radius, innerBackgroundRadius, outerBackgroundRadius);

        const double slicePoint = radius
                                  / 2; // set to be half way through the radius.
        peak.setSlicePoint(slicePoint);

        // View Settings Scale 1:1 on both x and y for simplicity.
        PeakRepresentationViewInformation viewInformation;
        viewInformation.viewHeight = 1.0;
        viewInformation.viewWidth = 1.0;
        viewInformation.windowHeight = 1.0;
        viewInformation.windowWidth = 1.0;
        viewInformation.xOriginWindow = 1;
        viewInformation.yOriginWindow = 1;

        // Act
        auto drawingInformation
            = peak.getDrawingInformationWrapper(viewInformation);

        // Assert
        auto drawingInformationSphere
            = std::static_pointer_cast<PeakPrimitiveCircle>(
                drawingInformation);

        // Quick white-box calculations of the outputs to expect.
        const double expectedOpacityAtDistance = (0.8 - 0) / 2;
        auto peakRadSQ = std::pow(radius, 2);
        auto planeDistanceSQ = std::pow((slicePoint - origin.Z()), 2);
        const double expectedRadius = std::sqrt(peakRadSQ - planeDistanceSQ);

        TS_ASSERT_EQUALS(expectedOpacityAtDistance,
                         drawingInformationSphere->peakOpacityAtDistance);
        TS_ASSERT_EQUALS(expectedRadius,
                         drawingInformationSphere->peakInnerRadiusX);
        TS_ASSERT_EQUALS(expectedRadius,
                         drawingInformationSphere->peakInnerRadiusY);
    }

    void test_move_position_produces_correct_position()
    {
        // Arrange
        MockPeakTransform *pMockTransform = new MockPeakTransform;
        EXPECT_CALL(*pMockTransform, transform(_))
            .Times(1)
            .WillOnce(Return(Mantid::Kernel::V3D(0, 0, 0)));
        PeakTransform_sptr transform(pMockTransform);

        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;
        const double innerBackgroundRadius = 2;
        const double outerBackgroundRadius = 3;
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act
        peak.movePosition(transform);

        // Assert
        TS_ASSERT(Mock::VerifyAndClearExpectations(pMockTransform));
    }

    void test_getBoundingBox()
    {
        /*

        width = height = outerradius * 2
        |---------------|
        |               |
        |               |
        |     (0,0)     |
        |               |
        |               |
        |---------------|

        */
        // Arrrange
        const Mantid::Kernel::V3D origin(0, 0, 0);
        const double radius = 1;                // Not important
        const double innerBackgroundRadius = 2; // Not important
        const double outerBackgroundRadius
            = 3; // This should be used to control the bounding box.
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act
        const auto boundingBox = peak.getBoundingBox();

        // Assert
        const double expectedLeft(origin.X() - outerBackgroundRadius);
        const double expectedBottom(origin.Y() - outerBackgroundRadius);
        const double expectedRight(origin.X() + outerBackgroundRadius);
        const double expectedTop(origin.Y() + outerBackgroundRadius);

        TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
        TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
        TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
        TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
    }

    void test_getBoundingBox_with_offset_origin()
    {
        /*

        width = height = outerradius * 2
        |---------------|
        |               |
        |               |
        |     (-1,1)    |
        |               |
        |               |
        |---------------|

        */
        // Arrange
        const Mantid::Kernel::V3D origin(-1, 1,
                                         0);    // Offset origin from (0, 0, 0)
        const double radius = 1;                // Not important
        const double innerBackgroundRadius = 2; // Not important
        const double outerBackgroundRadius
            = 3; // This should be used to control the bounding box.
        PeakRepresentationSphere peak(origin, radius, innerBackgroundRadius,
                                      outerBackgroundRadius);

        // Act
        auto boundingBox = peak.getBoundingBox();

        // Assert
        const double expectedLeft(origin.X() - outerBackgroundRadius);
        const double expectedBottom(origin.Y() - outerBackgroundRadius);
        const double expectedRight(origin.X() + outerBackgroundRadius);
        const double expectedTop(origin.Y() + outerBackgroundRadius);

        TS_ASSERT_EQUALS(expectedLeft, boundingBox.left());
        TS_ASSERT_EQUALS(expectedRight, boundingBox.right());
        TS_ASSERT_EQUALS(expectedTop, boundingBox.top());
        TS_ASSERT_EQUALS(expectedBottom, boundingBox.bottom());
    }
};

class PeakRepresentationSphereTestPerformance : public CxxTest::TestSuite
{
public:
    /**
    Here we create a distribution of Peaks. Peaks are dispersed. This is to give
    a measurable peformance.
    */
    PeakRepresentationSphereTestPerformance()
    {
        const int sizeInAxis = 100;
        const double radius = 5;
        const double innerBackgroundRadius = 6;
        const double outerBackgroundRadius = 7;
        m_peaks.reserve(sizeInAxis * sizeInAxis * sizeInAxis);
        for (int x = 0; x < sizeInAxis; ++x) {
            for (int y = 0; y < sizeInAxis; ++y) {
                for (int z = 0; z < sizeInAxis; ++z) {
                    Mantid::Kernel::V3D peakOrigin(x, y, z);
                    m_peaks.push_back(
                        boost::make_shared<PeakRepresentationSphereExposeProtectedWrapper>(
                            peakOrigin, radius, innerBackgroundRadius,
                            outerBackgroundRadius));
                }
            }
        }

        PeakRepresentationViewInformation viewInformation;
        viewInformation.viewHeight = 1.0;
        viewInformation.viewWidth = 1.0;
        viewInformation.windowHeight = 1.0;
        viewInformation.windowWidth = 1.0;
        viewInformation.xOriginWindow = 1;
        viewInformation.yOriginWindow = 1;

        m_viewInformation = viewInformation;
    }

    /// Test the performance of just setting the slice point.
    void test_setSlicePoint_performance()
    {
        for (double z = 0; z < 100; z += 5) {
            VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
            while (it != m_peaks.end()) {
                auto physicalPeak = *it;
                //physicalPeak->setSlicePoint(z);
                ++it;
            }
        }
    }

    /// Test the performance of just drawing.
    void test_draw_performance()
    {
        const int nTimesRedrawAll = 20;
        int timesDrawn = 0;
        while (timesDrawn < nTimesRedrawAll) {
            // Set the slicing point on all peaks.
            VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
            while (it != m_peaks.end()) {
                //it->getDrawingInformationWrapper(m_viewInformation);
                ++it;
            }
            ++timesDrawn;
        }
    }

    /// Test the performance of both setting the slice point and drawing..
    void test_whole_performance()
    {
        VecPeaksRepresentationSphere::iterator it = m_peaks.begin();
        //const double z = 10;
        while (it != m_peaks.end()) {
            //it->setSlicePoint(z);
            //it->getDrawingInformationWrapper(m_viewInformation);
            ++it;
        }
    }

private:
    typedef boost::shared_ptr<PeakRepresentationSphereExposeProtectedWrapper>
        PeaksRepresentationSphere_sptr;
    typedef std::vector<PeaksRepresentationSphere_sptr>
        VecPeaksRepresentationSphere;

    /// Collection to store a large number of physicalPeaks.
    VecPeaksRepresentationSphere m_peaks;
    PeakRepresentationViewInformation m_viewInformation;
};

#endif
