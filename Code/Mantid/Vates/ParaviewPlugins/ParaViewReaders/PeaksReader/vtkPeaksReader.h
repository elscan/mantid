#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "vtkPolyDataAlgorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkPeaksReader : public vtkPolyDataAlgorithm
{
public:
  static vtkPeaksReader *New();
  vtkTypeRevisionMacro(vtkPeaksReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetWidth(double width);
  void SetDimensions(int dimensions);
  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress, const std::string& message);
  /// Getter for the workspace type
  char* GetWorkspaceTypeName();

protected:
  vtkPeaksReader();
  ~vtkPeaksReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  
  vtkPeaksReader(const vtkPeaksReader&);
  
  void operator = (const vtkPeaksReader&);

  //Peak width;
  double m_width;

  /// File name from which to read.
  char *FileName;

  /// Flag indicates when set up is complete wrt  the conversion of the nexus file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;
  
  /// Cached PeaksWs Name
  std::string m_wsTypeName;

  /// Cached PeaksWS
  Mantid::API::IPeaksWorkspace_sptr  m_PeakWS;

  /// Int representing an enum for q_lab, etc.
  int m_dimensions;

  

};
#endif
