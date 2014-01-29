#ifndef MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_
#define MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/ComptonScatteringCountRate.h"
#include "ComptonProfileTestHelpers.h"

class ComptonScatteringCountRateTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComptonScatteringCountRateTest *createSuite() { return new ComptonScatteringCountRateTest(); }
  static void destroySuite( ComptonScatteringCountRateTest *suite ) { delete suite; }

  void test_Function_Has_Expected_Intensity_Attribute_And_No_Parameters()
  {
    auto countRate = createFunction();

    TS_ASSERT(countRate->nAttributes() > 1);
    TS_ASSERT_THROWS_NOTHING(countRate->getAttribute("IntensityConstraints"));
    TS_ASSERT_EQUALS(0, countRate->nParams());
  }

  void test_Empty_String_For_Intensity_Attribute_Throws_Error()
  {
    auto countRate = createFunction();

    TS_ASSERT_THROWS(countRate->setAttributeValue("IntensityConstraints", ""), std::invalid_argument);
  }

  void test_Incorrect_String_For_Intensity_Attribute_Throws_Error()
  {
    auto countRate = createFunction();

    TS_ASSERT_THROWS(countRate->setAttributeValue("IntensityConstraints", "Matrix"), std::invalid_argument);
  }

  void test_Single_Row_In_Intensity_Attribute_Does_Not_Throw()
  {
    auto countRate = createFunction();

    // Single row
    TS_ASSERT_THROWS_NOTHING(countRate->setAttributeValue("IntensityConstraints", "Matrix(1,4)0|1|0|4"));
  }

  void test_Multiple_Rows_In_Intensity_Attribute_Does_Not_Throw()
  {
    auto countRate = createFunction();
    // Multiple rows
    TS_ASSERT_THROWS_NOTHING(countRate->setAttributeValue("IntensityConstraints", "Matrix(2,4)0|1|0|4|0|0|2|5"));
  }

  void test_Function_Accepts_Having_No_Equality_Constraints_When_Setting_Workspace()
  {
    using namespace Mantid::API;
    IFunction_sptr func = createFunctionNoBackground(); // No equality matrix set
    double x0(165.0),x1(166.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);

    TS_ASSERT_THROWS_NOTHING(func->setWorkspace(testWS));
  }

  void test_Function_With_No_Background_Gives_Expected_Results_Given_Test_Data()
  {
     using namespace Mantid::API;
     IFunction_sptr func = createFunctionNoBackground();
     double x0(165.0),x1(166.0),dx(0.5);
     auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
     auto & dataX = testWS->dataX(0);
     std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds
     func->setWorkspace(testWS);
     FunctionDomain1DView domain(dataX.data(), dataX.size());
     FunctionValues values(domain);

     TS_ASSERT_THROWS_NOTHING(func->function(domain, values));

     const double tol(1e-10);
     TS_ASSERT_DELTA(2, values.getCalculated(0), tol); // Each member returns 1
     TS_ASSERT_DELTA(2, values.getCalculated(1), tol);
     TS_ASSERT_DELTA(2, values.getCalculated(2), tol);
  }

  void test_Function_Including_Background_Gives_Expected_Results_Given_Test_Data()
  {
     using namespace Mantid::API;
     IFunction_sptr func = createFunctionWithBackground();
     double x0(165.0),x1(166.0),dx(0.5);
     auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
     auto & dataX = testWS->dataX(0);
     std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds
     func->setWorkspace(testWS);

     FunctionDomain1DView domain(dataX.data(), dataX.size());
     FunctionValues values(domain);

     TS_ASSERT_THROWS_NOTHING(func->function(domain, values));

     const double tol(1e-10);
     TS_ASSERT_DELTA(2.25, values.getCalculated(0), tol); // Each member returns 1
     TS_ASSERT_DELTA(2.25, values.getCalculated(1), tol);
     TS_ASSERT_DELTA(2.25, values.getCalculated(2), tol);
  }


  void test_Iteration_Starting_Resets_Intensity_Parameters_Correctly_Without_Equality_Matrix()
  {
    using namespace Mantid::API;
    IFunction_sptr func = createFunctionNoBackground();
    double x0(165.0),x1(166.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
    auto & dataX = testWS->dataX(0);
    std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds
    func->setWorkspace(testWS);

    func->iterationStarting();
    TS_ASSERT_DELTA(func->getParameter(0),5.0, 1e-10);
    TS_ASSERT_DELTA(func->getParameter(1),0.5, 1e-10);
    TS_ASSERT_DELTA(func->getParameter(2),10.0, 1e-10);
    TS_ASSERT_DELTA(func->getParameter(3),0.5, 1e-10);
  }

  void test_Iteration_Starting_Resets_Intensity_Parameters_Satisfying_Equality_Matrix()
  {
    using namespace Mantid::API;
    IFunction_sptr func = createFunctionNoBackground();
    func->setAttributeValue("IntensityConstraints", "Matrix(1|2)1|-2"); // I_1 = 2I_2

    double x0(165.0),x1(166.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
    auto & dataX = testWS->dataX(0);
    std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds
    func->setWorkspace(testWS);

    func->iterationStarting();
    TS_ASSERT_DELTA(func->getParameter(0),5.0, 1e-10); // width_1
    TS_ASSERT_DELTA(func->getParameter(1),0.6666666633, 1e-10); // I_1
    TS_ASSERT_DELTA(func->getParameter(2),10.0, 1e-10); // width_2
    TS_ASSERT_DELTA(func->getParameter(3),0.3333333317, 1e-10); //I_2
  }

  void test_Iteration_Starting_Resets_Intensity_Parameters_When_Number_Intensity_Pars_Does_Not_Match_Number_Masses()
  {
    using namespace Mantid::API;
    const bool useTwoIntensityFuncAsFirst = true;
    IFunction_sptr func = createFunctionNoBackground(useTwoIntensityFuncAsFirst);

    double x0(165.0),x1(166.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
    auto & dataX = testWS->dataX(0);
    std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds

    func->setWorkspace(testWS);

    func->iterationStarting();
    TS_ASSERT_DELTA(func->getParameter(0),5.0, 1e-10); // width_1
    TS_ASSERT_DELTA(func->getParameter(1),0.33333333, 1e-8); // first mass intensity 1
    TS_ASSERT_DELTA(func->getParameter(2),0.33333333, 1e-8); // first mass intensity 2
    TS_ASSERT_DELTA(func->getParameter(3),10.0, 1e-10); // width_2
    TS_ASSERT_DELTA(func->getParameter(4),0.33333333, 1e-8); // second mass intensity
  }


  void xtest_Iteration_Starting_Resets_Intensity_Parameters_And_Background_Parameters_With_Background_Included()
  {
    using namespace Mantid::API;
    IFunction_sptr func = createFunctionWithBackground();
    func->setAttributeValue("IntensityConstraints", "Matrix(1|2)1|-2"); // I_1 = 2I_2

    double x0(165.0),x1(166.0),dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1,x0,x1,dx);
    auto & dataX = testWS->dataX(0);
    std::transform(dataX.begin(), dataX.end(), dataX.begin(), std::bind2nd(std::multiplies<double>(),1e-06)); // to seconds

    func->setWorkspace(testWS);

    func->iterationStarting();
//    TS_ASSERT_DELTA(func->getParameter(0),5.0, 1e-10); // width_1
//    TS_ASSERT_DELTA(func->getParameter(1),-0.0506748344, 1e-10); // I_1
//    TS_ASSERT_DELTA(func->getParameter(2),10.0, 1e-10); //width_2
//    TS_ASSERT_DELTA(func->getParameter(3),-0.0253374172, 1e-10); // I_2
//    TS_ASSERT_DELTA(func->getParameter(4),-0.0422290287, 1e-10); // background A_0
//    TS_ASSERT_DELTA(func->getParameter(5),0.00675680781, 1e-10); // background A_1
  }


private:

  /// A simple working object to use for the testing
  /// Provides a canned answer of 1 for the massProfile
  class ComptonProfileStub : public Mantid::CurveFitting::ComptonProfile
  {
  public:
    ComptonProfileStub() : ComptonProfile()
    {
      declareParameter("Width",1.0);
      declareParameter("Intensity",1.0);
    }
    std::string name() const { return "ComptonProfileStub"; }
    std::vector<size_t> intensityParameterIndices() const { return std::vector<size_t>(1,1); }

    size_t fillConstraintMatrix(Mantid::Kernel::DblMatrix & cmatrix,const size_t start,
                                const std::vector<double>&) const
    {
      for(size_t i = 0; i < cmatrix.numRows(); ++i)
      {
        cmatrix[i][start] = 1.0;
      }
      return 1;
    }

    void massProfile(double * result, const size_t nData) const
    {
      for(size_t i = 0; i < nData; ++i) result[i] = 1;
    }
  };

  /// A simple working object that has 2 intensity parameters to use for the testing
  /// Provides a canned answer of 1 for the massProfile
  class TwoIntensitiesComptonProfileStub : public ComptonProfileStub
  {
  public:
    TwoIntensitiesComptonProfileStub() : ComptonProfileStub()
    {
      declareParameter("Intensity_2",1.0);
    }
    std::string name() const { return "TwoIntensitiesComptonProfileStub"; }
    std::vector<size_t> intensityParameterIndices() const
    {
      std::vector<size_t> indices(2,1); // index 1
      indices[1] = 2; // index 2
      return indices;
    }
    size_t fillConstraintMatrix(Mantid::Kernel::DblMatrix & cmatrix,const size_t start,
                                const std::vector<double>&) const
    {
      for(size_t i = 0; i < cmatrix.numRows(); ++i)
      {
        for(size_t j = start; j < start + 2; ++j)
        {
          cmatrix[i][j] = 1.0;
        }
      }
      return 2; //used 2 columns
    }
  };


  /// Background impl for testing
  /// Retursns canned answer of 0.25
  class LinearStub : public Mantid::API::IFunction1D, Mantid::API::ParamFunction
  {
  public:
    LinearStub()
    {
      declareAttribute("n",Mantid::API::IFunction::Attribute(1));
      declareParameter("A0",1.0);
      declareParameter("A1",1.0);
    }
    std::string name() const { return "LinearStub"; }
    void function1D(double* out, const double*, const size_t nData) const
    {
      for(size_t i = 0; i < nData; ++i) out[i] = 0.25;
    }
  };

  Mantid::API::IFunction_sptr createFunctionWithBackground()
  {
    auto func = createFunctionNoBackground();

    auto background = boost::make_shared<LinearStub>();
    background->initialize();
    func->addFunction(background);
    func->setUpForFit();

    return func;
  }

  boost::shared_ptr<Mantid::CurveFitting::ComptonScatteringCountRate>
  createFunctionNoBackground(const bool useTwoIntensityFuncAsFirst = false)
  {
    boost::shared_ptr<ComptonProfileStub> func1;
    if(useTwoIntensityFuncAsFirst)
    {
      func1 = boost::make_shared<TwoIntensitiesComptonProfileStub>();
      func1->initialize();
      func1->setParameter("Intensity_2", 3.0);
    }
    else
    {
      func1 = boost::make_shared<ComptonProfileStub>();
      func1->initialize();
    }
    func1->setAttributeValue("WorkspaceIndex",0);
    func1->setAttributeValue("Mass",1.0);
    func1->setParameter("Width", 5.0);
    func1->setParameter("Intensity", 2.0);


    auto func2 = boost::make_shared<ComptonProfileStub>();
    func2->initialize();
    func2->setAttributeValue("WorkspaceIndex",0);
    func2->setAttributeValue("Mass",1.0);
    func2->setParameter("Width", 10.0);
    func2->setParameter("Intensity", 3.0);

    using Mantid::CurveFitting::ComptonScatteringCountRate;
    auto profile = boost::make_shared<ComptonScatteringCountRate>();
    profile->initialize();
    profile->addFunction(func1);
    profile->addFunction(func2);
    profile->setUpForFit();

    return profile;
  }

  Mantid::API::IFunction_sptr createFunction()
  {
    using Mantid::CurveFitting::ComptonScatteringCountRate;

    auto profile = boost::make_shared<ComptonScatteringCountRate>();
    profile->initialize();
    return profile;
  }

};


#endif /* MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATETEST_H_ */
