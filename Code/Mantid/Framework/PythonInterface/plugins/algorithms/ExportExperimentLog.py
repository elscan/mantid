import mantid.simpleapi as api
import mantid
from mantid.api import *
from mantid.kernel import *

import os
import datetime

class ExportExperimentLog(PythonAlgorithm):

    """ Algorithm to export experiment log
    """

    def summmary(self):
        return "Exports experimental log."

    def PyInit(self):
        """ Declaration of properties
        """
        # wsprop = mantid.api.MatrixWorkspaceProperty("InputWorkspace", "", mantid.kernel.Direction.Input)
        wsprop = MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input)
        self.declareProperty(wsprop, "Input workspace containing the sample log information. ")

        self.declareProperty(FileProperty("OutputFilename","", FileAction.Save, ['.txt, .csv']),
            "Output file of the experiment log.")

        filemodes = ["append", "fastappend", "new"]
        self.declareProperty("FileMode", "append", mantid.kernel.StringListValidator(filemodes),
            "Optional to create a new file or append to an existing file.")

        lognameprop = StringArrayProperty("SampleLogNames", values=[], direction=Direction.Input)
        self.declareProperty(lognameprop, "Sample log names.")

        logtitleprop = StringArrayProperty("SampleLogTitles", values=[], direction=Direction.Input)
        self.declareProperty(logtitleprop, "Log sample titles as file header.")

        logoperprop = StringArrayProperty("SampleLogOperation", values=[], direction=Direction.Input)
        self.declareProperty(logoperprop, "Operation on each log, including None (as no operation), min, max, average, sum and \"0\". ")

        fileformates = ["tab", "comma (csv)"]
        des = "Output file format.  'tab' format will insert a tab between 2 adjacent values; 'comma' will put a , instead. " + \
                "With this option, the posfix of the output file is .csv automatically. "
        self.declareProperty("FileFormat", "tab", mantid.kernel.StringListValidator(fileformates), des)

        self.declareProperty("OrderByTitle", "", "Log file will be ordered by the value of this title from low to high.") 

        overrideprop = StringArrayProperty("OverrideLogValue", values=[], direction=Direction.Input)
        self.declareProperty(overrideprop, "List of paired strings as log title and value to override values from workspace.")

        # Time zone
        timezones = ["UTC", "America/New_York"]
        self.declareProperty("TimeZone", "America/New_York", StringListValidator(timezones))


        return


    def PyExec(self):
        """ Main execution body
        """
        # Process inputs
        self._processInputs()

        # Get sample log's value
        valuedict = self._getSampleLogsValue()
        if valuedict is None:
            return

        # Load input
        if self._filemode == "append":
            same = self._examineLogFile()
            if same is False:
                if self._reorderOld is True:
                    self._reorderExistingFile()
                else:
                    self._startNewFile()
                # ENDIFELSE
            # ENDIF (same)
        # ENDIF (filemode)

        # Write file
        if self._filemode == "new":
            # Create log file and write header
            self._createLogFile()

        # Append the new experiment log
        self._appendExpLog(valuedict)

        # Order the record file
        if self._orderRecord is True:
            self._orderRecordFile()

        return

    def _processInputs(self):
        """ Process input properties
        """
        import os
        import os.path

        self._wksp = self.getProperty("InputWorkspace").value

        self._logfilename = self.getProperty("OutputFilename").value

        # Field and keys
        self._headerTitles = self.getProperty("SampleLogTitles").value
        self._sampleLogNames = self.getProperty("SampleLogNames").value
        ops = self.getProperty("SampleLogOperation").value

        if len(self._sampleLogNames) != len(ops):
            raise NotImplementedError("Size of sample log names and sample operations are unequal!")
        self._sampleLogOperations = []
        for i in xrange(len(self._sampleLogNames)):
            value = ops[i]
            self._sampleLogOperations.append(value)
        # ENDFOR

        if len(self._headerTitles) > 0 and len(self._headerTitles) != len(self._sampleLogNames):
            raise NotImplementedError("Input header titles have a different length to sample log names")

        # Output file format
        self._fileformat = self.getProperty("FileFormat").value
        if self._fileformat == "tab":
            self._valuesep = "\t"
        else:
            self._valuesep = ","

        # Output file's postfix
        if self._fileformat == "comma (csv)":
            fileName, fileExtension = os.path.splitext(self._logfilename)
            if fileExtension != ".csv":
                # Force the extension of the output file to be .csv
                self._logfilename = "%s.csv" % (fileName)

        # Determine file mode
        if os.path.exists(self._logfilename) is False:
            self._filemode = "new"
            if len(self._headerTitles) == 0:
                raise NotImplementedError("Without specifying header title, unable to new a file.")
            self.log().debug("[DB] Log file %s does not exist. So file mode is NEW." % (self._logfilename))
        else:
            self._filemode = self.getProperty("FileMode").value
            self.log().debug("[DB] FileMode is from user specified value.")

        # Examine the file mode
        if self._filemode == "new" or self._filemode == "append":
            if len(self._headerTitles) != len(self._sampleLogNames):
                raise NotImplementedError("In mode new or append, there must be same number of sample titles and names")

        self.log().notice("File mode is %s. " % (self._filemode))

        # This is left for a feature that might be needed in future.
        self._reorderOld = False

        self._timezone = self.getProperty("TimeZone").value

        # Determine whether output log-record file should be ordered by value of some log
        self._orderRecord = False
        self._titleToOrder = None
        if self._filemode != "new":
            ordertitle = self.getProperty("OrderByTitle").value 
            if ordertitle in self._headerTitles:
                self._orderRecord = True
                self.titleToOrder = ordertitle
            else:
                self.log().warning("Specified title to order by (%s) is not in given log titles." % (ordertitle))

        # Override log values: it will not work in fastappend mode to override
        overridelist = self.getProperty("OverrideLogValue").value
        if len(self._headerTitles) > 0:
            if len(overridelist) % 2 != 0:
                raise NotImplementedError("Number of items in OverrideLogValue must be even.")
            self._ovrdTitleValueDict = {}
            for i in xrange(len(overridelist)/2):
                title = overridelist[2*i]
                if title in self._headerTitles:
                    self._ovrdTitleValueDict[title] = overridelist[2*i+1]
                else:
                    self.log().warning("Override title %s is not recognized. " % (title))

        return

    def _createLogFile(self):
        """ Create a log file
        """
        if len(self._headerTitles) == 0:
            raise NotImplementedError("No header title specified. Unable to write a new file.")

        wbuf = ""
        for ititle in xrange(len(self._headerTitles)):
            title = self._headerTitles[ititle]
            wbuf += "%s" % (title)
            if ititle < len(self._headerTitles)-1:
                wbuf += self._valuesep

        try:
            ofile = open(self._logfilename, "w")
            ofile.write(wbuf)
            ofile.close()
        except OSError as err:
            raise NotImplementedError("Unable to write file %s. Check permission. Error message %s." % (
                self._logfilename, str(err)))

        return

    def _examineLogFile(self):
        """ Examine existing log file.
        If the titles in the existing log file are different from the inputs,
        then the old file will be saved
        """
        # Parse old file
        try:
            logfile = open(self._logfilename, "r")
            lines = logfile.readlines()
            logfile.close()
        except OSError as err:
            raise NotImplementedError("Unable to read existing log file %s. Error: %s." % (
                self._logfilename, str(err)))

        # Find the title line: first none-empty line
        for line in lines:
            titleline = line.strip()
            if len(titleline) > 0:
                break

        # Examine
        titles = titleline.split()
        self.log().notice("[DB] Examine finds titles: %s" % (titles))

        same = True
        if len(titles) != len(self._headerTitles):
            if len(self._headerTitles) == 0:
                self._headerTitles = titles[:]
            else: 
                same = False
        for ititle in xrange(len(titles)):
            title1 = titles[ititle]
            title2 = self._headerTitles[ititle]
            if title1 != title2:
                same = False
                break
            # ENDIF
        # ENDFOR

        return same


    def _startNewFile(self):
        """ Start a new file is user wants and save the older one to a different name
        """
        import datetime
        import time
        import os

        # Rename old file and reset the file mode

        # Rename the old one: split path from file, new name, and rename
        fileName, fileExtension = os.path.splitext(self._logfilename)

        now = datetime.datetime.now()
        nowstr = time.strftime("%Y_%B_%d_%H_%M")

        newfilename = fileName + "_" + nowstr + fileExtension
        os.rename(self._logfilename, newfilename)

        # Reset the file mode
        self._filemode = "new"

        return


    def _appendExpLog(self, logvaluedict):
        """ Append experiment log values to log file
        """
        self.log().information("Appending is called once.")

        # Write to a buffer
        wbuf = ""

        self.log().notice("[DB] Samlpe Log Names: %s" % (self._sampleLogNames))
        self.log().notice("[DB] Title      Names: %s" % (self._headerTitles))

        if len(self._headerTitles) == 0:
            skip = True
        else:
            skip = False
        
        headertitle = None
        for il in xrange(len(self._sampleLogNames)):
            if skip is False: 
                headertitle = self._headerTitles[il]
            if headertitle is not None and headertitle in self._ovrdTitleValueDict.keys():
                # overriden
                value = self._ovrdTitleValueDict[headertitle]

            else:
                # from input workspace
                logname = self._sampleLogNames[il]
                optype = self._sampleLogOperations[il]
                key = logname + "-" + optype
                if key in logvaluedict.keys():
                    value = logvaluedict[key]
                elif logname in logvaluedict.keys():
                    value = logvaluedict[logname]
            # ENDIFELSE

            wbuf += "%s%s" % (str(value), self._valuesep)
        wbuf = wbuf[0:-1]

        # Append to file
        lfile = open(self._logfilename, "a")
        lfile.write("\n%s" %(wbuf))
        lfile.close()

        return

    def _orderRecordFile(self):
        """ Check and order (if necessary) record file 
        by value of specified log by title
        """
        self.log().notice("[DB] Order Record File!")

        # Read line
        lfile = open(self._logfilename, "r")
        lines = lfile.readlines()
        lfile.close()

        # Create dictionary for the log value
        titlelines = []
        linedict = {}
        ilog = self._headerTitles.index(self.titleToOrder)

        for line in lines:
            cline = line.strip()
            if len(cline) == 0:
                continue

            if cline.startswith(self._headerTitles[0]) is True or cline.startswith('#'):
                # title line or comment line
                titlelines.append(line)
            else:
                # value line
                try: 
                    keyvalue = line.split(self._valuesep)[ilog]
                except IndexError:
                    self.log().error("Order record failed.")
                    return
                linedict[keyvalue] = line
            # ENDIFELSE
        # ENDFOR

        # Check needs to re-order
        if linedict.keys() != sorted(linedict.keys()):
            # Re-write file
            wbuf = ""
            
            # title line
            for line in titlelines:
                wbuf += line

            # log entry line
            iline = 0
            numlines = len(linedict.keys())
            for ivalue in sorted(linedict.keys()):
                wbuf += linedict[ivalue]
                # Add extra \n in case reordered
                if iline != numlines-1 and wbuf[-1] != '\n':
                    wbuf += '\n'
                iline += 1
            # ENDFOR

            # Last line should not ends with \n
            if wbuf[-1] == '\n':
                wbuf = wbuf[0:-1]

            # Remove unsupported character which may cause importing error of GNUMERIC
            wbuf = wbuf.translate(None, chr(0))

            # Re-write file
            ofile = open(self._logfilename, "w")
            ofile.write(wbuf)
            ofile.close()

        # ENDIF

        return


    def _reorderExistingFile(self):
        """ Re-order the columns of the existing experimental log file
        """
        raise NotImplementedError("Too complicated")

        return


    def _getSampleLogsValue(self):
        """ From the workspace to get the value
        """
        import numpy as np

        valuedict = {}
        run = self._wksp.getRun()
        if run is None:
            self.log().error("There is no Run object associated with workspace %s. " % (
                str(self._wksp)))
            return None

        #for logname in self._sampleLogNames:
        for il in xrange(len(self._sampleLogNames)):
            logname = self._sampleLogNames[il]
            isexist = run.hasProperty(logname)

            operationtype = self._sampleLogOperations[il]
            # check whether this property does exist.
            if isexist is False:
                # If not exist, use 0.00 as default
                self.log().warning("Sample log %s does not exist in given workspace %s. " % (logname, str(self._wksp)))
                valuedict[logname] = 0.0
                continue

            logproperty = run.getProperty(logname)
            logclass = logproperty.__class__.__name__

            # Get log value according to type
            if logclass == "StringPropertyWithValue":
                propertyvalue = logproperty.value
                # operationtype = self._sampleLogOperations[il]
                if operationtype.lower() == "localtime":
                    propertyvalue = self._convertLocalTimeString(propertyvalue)
            elif logclass == "FloatPropertyWithValue":
                propertyvalue = logproperty.value
            elif logclass == "FloatTimeSeriesProperty":
                # operationtype = self._sampleLogOperations[il]
                if operationtype.lower() == "min":
                    propertyvalue = min(logproperty.value)
                elif operationtype.lower() == "max":
                    propertyvalue = max(logproperty.value)
                elif operationtype.lower() == "average":
                    propertyvalue = np.average(logproperty.value)
                elif operationtype.lower() == "sum":
                    propertyvalue = np.sum(logproperty.value)
                elif operationtype.lower() == "0":
                    propertyvalue = logproperty.value[0]
                else:
                    raise NotImplementedError("Operation %s for FloatTimeSeriesProperty %s is not supported." % (operationtype, logname))
            else:
                raise NotImplementedError("Class type %d is not supported." % (logclass))

            key = logname + "-" + operationtype
            valuedict[key] = propertyvalue
        # ENDFOR

        return valuedict


    def _convertLocalTimeString(self, utctimestr):
        """ Convert a UTC time in string to the local time in string
        """
        from datetime import datetime
        from dateutil import tz

        utctimestr = str(utctimestr)

        self.log().information("Input UTC time = %s" % (utctimestr))

        from_zone = tz.gettz('UTC')
        to_zone = tz.gettz(self._timezone)

        try:
            if utctimestr.count(".") == 1:
                tail = utctimestr.split(".")[1]
                extralen = len(tail)-6
                extra = utctimestr[-extralen:]
                utctimestr = utctimestr[0:-extralen]
            utctime = datetime.strptime(utctimestr, '%Y-%m-%dT%H:%M:%S.%f')
        except ValueError as err:
            self.log().error("Unable to convert time string %s. Error message: %s" % (utctimestr, str(err)))
            raise err

        utctime = utctime.replace(tzinfo=from_zone)
        localtime = utctime.astimezone(to_zone)

        localtimestr = localtime.strftime("%Y-%m-%d %H:%M:%S.%f") + extra

        return localtimestr

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ExportExperimentLog)
