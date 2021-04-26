# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path
from os import makedirs

from mantid.simpleapi import logger, Load, SaveNexus, NormaliseByCurrent, Integration, AppendSpectra, DeleteWorkspace, \
    RenameWorkspace
from mantid.simpleapi import AnalysisDataService as Ads

from Engineering.gui.engineering_diffraction.tabs.common import path_handling
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"

VANADIUM_DIRECTORY_NAME = "Vanadium_Runs"

SAVED_FILE_CURVE_SUFFIX = "_precalculated_vanadium_run_bank_curves.nxs"
SAVED_FILE_INTEG_SUFFIX = "_precalculated_vanadium_run_integration.nxs"


def fetch_correction_workspaces(vanadium_path, instrument, rb_num=""):
    """
    Fetch workspaces from the file system or create new ones.
    :param vanadium_path: The path to the requested vanadium run raw data.
    :param instrument: The instrument the data came from.
    :param rb_num: A user identifier, usually an experiment number.
    :return: The resultant integration and curves workspaces.
    """
    vanadium_number = path_handling.get_run_number_from_path(vanadium_path, instrument)
    integ_path = generate_van_ws_file_path(vanadium_number, SAVED_FILE_INTEG_SUFFIX, rb_num)
    curves_path = generate_van_ws_file_path(vanadium_number, SAVED_FILE_CURVE_SUFFIX, rb_num)
    force_recalc = get_setting(path_handling.INTERFACES_SETTINGS_GROUP,
                               path_handling.ENGINEERING_PREFIX, "recalc_vanadium", return_type=bool)
    if path.exists(curves_path) and path.exists(integ_path) and not force_recalc:  # Check if the cached files exist.
        try:
            integ_workspace = Load(Filename=integ_path, OutputWorkspace=INTEGRATED_WORKSPACE_NAME)
            curves_workspace = Load(Filename=curves_path, OutputWorkspace=CURVES_WORKSPACE_NAME)
            if rb_num:
                user_integ = generate_van_ws_file_path(vanadium_number, SAVED_FILE_INTEG_SUFFIX, rb_num=rb_num)
                user_curves = generate_van_ws_file_path(vanadium_number, SAVED_FILE_CURVE_SUFFIX, rb_num=rb_num)
                if not path.exists(user_integ) and not path.exists(user_curves):
                    save_van_workspace(integ_workspace, user_integ)
                    save_van_workspace(curves_workspace, user_curves)
            return integ_workspace, curves_workspace
        except RuntimeError as e:
            logger.error(
                "Problem loading existing vanadium calculations. Creating new files. Description: "
                + str(e))
    integ_workspace = _calculate_vanadium_correction(vanadium_path)
    save_van_workspace(integ_workspace, integ_path)
    if rb_num:
        user_integ = generate_van_ws_file_path(vanadium_number, SAVED_FILE_INTEG_SUFFIX, rb_num=rb_num)
        save_van_workspace(integ_workspace, user_integ)
    return integ_workspace, None  # curves workspace, if not retrieved from fs, will be created and saved separately


def check_workspaces_exist():
    return Ads.doesExist(CURVES_WORKSPACE_NAME) and Ads.doesExist(INTEGRATED_WORKSPACE_NAME)


def _calculate_vanadium_correction(vanadium_path):
    """
    Runs the vanadium correction algorithm.
    :param vanadium_path: The path to the vanadium data.
    :return: The integrated workspace and the curves generated by the algorithm.
    """
    try:
        Load(Filename=vanadium_path, OutputWorkspace=VANADIUM_INPUT_WORKSPACE_NAME)
    except Exception as e:
        logger.error("Error when loading vanadium sample data. "
                     "Could not run Load algorithm with vanadium run number: "
                     + str(vanadium_path) + ". Error description: " + str(e))
        raise RuntimeError
    van_ws = Ads.Instance().retrieve(VANADIUM_INPUT_WORKSPACE_NAME)
    NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=van_ws)
    # sensitivity correction for van
    nbins = van_ws.blocksize()
    ws_van_int = Integration(InputWorkspace=van_ws)
    ws_van_int /= nbins
    RenameWorkspace(InputWorkspace=ws_van_int, OutputWorkspace=INTEGRATED_WORKSPACE_NAME)
    return ws_van_int


def save_van_workspace(workspace, output_path):
    """
    Attempt to save the created workspaces to the filesystem.
    :param workspace: The vanadium workspace
    :param output_path: The path to save the workspace to.
    """
    try:
        SaveNexus(InputWorkspace=workspace, Filename=output_path)
    except RuntimeError as e:  # If the files cannot be saved, continue with the execution of the algorithm anyway.
        logger.error(
            "Vanadium Integration file could not be saved to the filesystem. Description: " + str(e))
        return


def _generate_vanadium_saves_file_path(rb_num=""):
    """
    Generate file paths based on a vanadium run number.
    :param rb_num: User identifier, usually an experiment number.
    :return: The full path to the file.
    """
    if rb_num:
        vanadium_dir = path.join(path_handling.get_output_path(), "User", rb_num,
                                 VANADIUM_DIRECTORY_NAME)
    else:
        vanadium_dir = path.join(path_handling.get_output_path(), VANADIUM_DIRECTORY_NAME)
    if not path.exists(vanadium_dir):
        makedirs(vanadium_dir)
    return vanadium_dir


def generate_van_ws_file_path(vanadium_number, suffix, rb_num=""):
    """
    Generate file path based on a vanadium run number for the vanadium integration workspace.
    :param vanadium_number: The number of the vanadium run.
    :param suffix: The suffix the filename to be saved
    :param rb_num: User identifier, usually an experiment number.
    :return: The full path to the file.
    """
    integrated_filename = vanadium_number + suffix
    vanadium_dir = _generate_vanadium_saves_file_path(rb_num)
    return path.join(vanadium_dir, integrated_filename)


def handle_van_curves(van_curves, van_path, instrument, rb_num):
    # this method is provided to be used by the calibration model, as this is where the vanadium curves are generated
    van_number = path_handling.get_run_number_from_path(van_path, instrument)
    curves_path = generate_van_ws_file_path(van_number, SAVED_FILE_CURVE_SUFFIX, rb_num)
    if len(van_curves) == 2:
        curves_ws = AppendSpectra(InputWorkspace1=van_curves[0], InputWorkspace2=van_curves[1])
        DeleteWorkspace(van_curves[1])
    else:
        curves_ws = van_curves[0]
    #DeleteWorkspace(van_curves[0])
    save_van_workspace(curves_ws, curves_path)
    RenameWorkspace(InputWorkspace=curves_ws, OutputWorkspace=CURVES_WORKSPACE_NAME)
