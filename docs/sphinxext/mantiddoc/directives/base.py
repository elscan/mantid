# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from docutils import statemachine
from docutils.parsers.rst import Directive  # pylint: disable=unused-import
import os
import re
from mantid.api import AlgorithmFactory, AlgorithmManager, FunctionFactory
from mantiddoc import get_logger

ALG_DOCNAME_RE = re.compile(r"^([A-Z][a-zA-Z0-9]+)-v([0-9][0-9]*)$")
FIT_DOCNAME_RE = re.compile(r"^([A-Z][a-zA-Z0-9]+)$")


# ----------------------------------------------------------------------------------------
def algorithm_name_and_version(docname):
    """
    Returns the name and version of an algorithm based on the name of the
    document supplied. The expected name of the document is "AlgorithmName-v?", which
    is the name of the file with the extension removed

    Arguments:
      docname (str): The name of the document as supplied by docutils. Can contain slashes to indicate a path

    Returns:
      tuple: A tuple containing two elements (name, version). In the case of a fit function the version is None.
    """
    # simple check to see if it is an algorithm or fit function
    is_alg = "algorithms" in docname
    is_fit = "fitfunctions" in docname

    # docname includes path, using forward slashes, from root of documentation directory
    docname = docname.split("/")[-1]

    # name for an algorithm
    if is_alg:
        match = ALG_DOCNAME_RE.match(docname)
        if not match or len(match.groups()) != 2:
            raise RuntimeError("Document filename '%s.rst' does not match the expected format: AlgorithmName-vX.rst" % docname)

        grps = match.groups()
        return (str(grps[0]), int(grps[1]))

    # name for a a fitfunction
    if is_fit:
        match = FIT_DOCNAME_RE.match(docname)
        if not match or len(match.groups()) != 1:
            raise RuntimeError("Document filename '%s.rst' does not match the expected format: FitFunctionName.rst" % docname)

        return (str(match.groups()[0]), None)

    # fail now
    raise RuntimeError("Failed to find name from document filename ")


# ----------------------------------------------------------------------------------------
class BaseDirective(Directive):
    """
    Contains shared functionality for Mantid custom directives.
    """

    has_content = False
    final_argument_whitespace = True

    rst_lines = None

    @property
    def screenshots_dir(self):
        """
        Return the directory to be used to store auto-generated screenshots
        """
        screenshots_dir = os.environ.get("SCREENSHOTS_DIR", None)
        if screenshots_dir is None or screenshots_dir == "":
            return None
        else:
            return screenshots_dir

    def add_rst(self, text):
        """
        Appends given reST into a managed list. It is NOT inserted into the
        document until commit_rst() is called

        Args:
          text (str): reST to track
        """
        if self.rst_lines is None:
            self.rst_lines = []

        self.rst_lines.extend(statemachine.string2lines(text))

    def commit_rst(self):
        """
        Inserts the currently tracked rst lines into the state_machine
        """
        self.state_machine.insert_input(self.rst_lines, self.source())
        self.rst_lines = []

    def source(self):
        """
        Returns the full path to the source document
        """
        return self.state.document.settings.env.docname

    def make_header(self, name, pagetitle=False, level=2):
        """
        Makes a ReStructuredText title from the algorithm's name.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          pagetitle (bool): If True, this sets the level to 1 (overriding any other value).
          level (int): 1-4 the level of the heading to be used.

        Returns:
          str: ReST formatted header with algorithm_name as content.
        """
        level_dict = {1: "=", 2: "-", 3: "#", 4: "^"}

        if pagetitle:
            level = 1
        if level not in level_dict:
            env = self.state.document.settings.env
            env.app.warn("base.make_header - Did not understand level " + str(level))
            level = 2

        line = "\n" + level_dict[level] * (len(name)) + "\n"

        if level == 1:
            return line + name + line
        else:
            return name + line


# ----------------------------------------------------------------------------------------


class AlgorithmBaseDirective(BaseDirective):
    """
    Specialized base directive for an algorithm
    """

    algm_name = None
    algm_version = None

    def run(self):
        """
        The main entry point that docutils calls.
        It calls self.execute to do the main work. If an
        algorithm doesn't exist then the directive is
        skipped a debug message is emitted

        Derived classes should override execute() and insert
        whatever rst they require with self.add_rst()
        """
        nodes = []
        skip_msg = self.skip()
        if skip_msg != "":
            self.add_rst("**ERROR: %s**" % skip_msg)
        else:
            nodes = self.execute()

        if self.rst_lines is not None:
            self.commit_rst()
        return nodes

    def skip(self):
        """
        Override and return a string depending on whether the directive
        should be skipped. If empty then the directive should be processed
        otherwise the string should contain the error message
        The default is to skip (and warn) if the algorithm is not known.

        Returns:
          str: Return error message string if the directive should be skipped
        """
        from mantid.api import FunctionFactory

        name, version = self.algorithm_name(), self.algorithm_version()
        msg = ""
        if version is None:  # it is a fit function
            if name in FunctionFactory.getFunctionNames():
                return ""
            else:
                msg = "No fit function '%s', skipping directive" % name
        else:
            if AlgorithmFactory.exists(name, version):
                return ""
            else:
                msg = "No algorithm '%s' version '%d', skipping directive" % (name, version)

        # warn the user
        if len(msg) > 0:
            logger = get_logger(__name__, self.state.document.settings.env.app)
            logger.verbose(msg)
        return msg

    def algorithm_name(self):
        """
        Returns the algorithm name as parsed from the document name
        """
        if self.algm_name is None:
            self._set_algorithm_name_and_version()
        return self.algm_name

    def algorithm_version(self):
        """
        Returns the algorithm version as parsed from the document name
        """
        if self.algm_version is None:
            self._set_algorithm_name_and_version()
        return self.algm_version

    def create_mantid_algorithm(self, algorithm_name, version=-1):
        """
        Create and initializes a Mantid algorithm.

        Args:
          algorithm_name (str): The name of the algorithm to use for the title.
          version (int): Version of the algorithm to create

        Returns:
          algorithm: An instance of a Mantid algorithm.
        """
        if version == -1:
            version = AlgorithmFactory.Instance().highestVersion(algorithm_name)
        alg = AlgorithmManager.Instance().createUnmanaged(algorithm_name, version)
        alg.initialize()
        return alg

    def create_mantid_ifunction(self, function_name):
        """
        Create and initiializes a Mantid IFunction.

        Args:
          function_name (str): The name of the function to use.

        Returns:
          ifunction: An instance of a Mantid IFunction
        """
        return FunctionFactory.createFunction(function_name)

    def _set_algorithm_name_and_version(self):
        """
        Returns the name and version of an algorithm based on the name of the
        document. The expected name of the document is "AlgorithmName-v?", which
        is the name of the file with the extension removed
        """
        (self.algm_name, self.algm_version) = algorithm_name_and_version(self.source())
