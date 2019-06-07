# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer

from mantidqt.utils import BlockQSignals
from mantidqt.widgets.plotconfigdialog import get_axes_names_dict, curve_in_ax
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties, hide_curve
from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget.presenter import ErrorbarsTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.presenter import LineTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.presenter import MarkerTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.view import CurvesTabWidgetView


class CurvesTabWidgetPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if not view:
            self.view = CurvesTabWidgetView(parent)
        else:
            self.view = view

        # Create sub-tab presenters and add them to the view
        self.line_tab = LineTabWidgetPresenter(parent=self.view)
        self.marker_tab = MarkerTabWidgetPresenter(parent=self.view)
        self.errorbars_tab = ErrorbarsTabWidgetPresenter(parent=self.view)
        self.add_tab_to_view(self.line_tab, "Line")
        self.add_tab_to_view(self.marker_tab, "Marker")
        self.add_tab_to_view(self.errorbars_tab, "Errobars")

        # Fill the fields in the view
        self.axes_names_dict = get_axes_names_dict(self.fig)
        self.curve_names_dict = {}
        self.populate_select_axes_combo_box()
        self.populate_select_curve_combo_box()
        self.update_view()

        # Signals
        self.view.select_axes_combo_box.currentIndexChanged.connect(
            self.populate_curve_combo_box_and_update_view)
        self.view.select_curve_combo_box.currentIndexChanged.connect(
            self.update_view)
        self.view.remove_curve_button.clicked.connect(
            self.remove_selected_curve)

    def add_tab_to_view(self, tab, name):
        """Add QWidget to the tab container with given name"""
        self.view.tab_container.addTab(tab.view, name)

    def apply_properties(self):
        """Take properties from views and set them on the selected curve"""
        view_props = self.get_view_curve_properties()
        curve = self.get_selected_curve()
        for tab in ['line_tab', 'marker_tab', 'errorbars_tab']:
            getattr(self, tab).apply_properties()
        self.set_curve_label(curve, view_props.label)
        hide_curve(curve, view_props.hide,
                   hide_bars=not self.errorbars_tab.view.get_hide())

    def close_tab(self):
        """Close the tab and set the view to None"""
        self.view.close()
        self.view = None

    def enable_tabs(self):
        """
        Enable/disable line, marker and errorbar tabs for selected curve
        """
        curve = self.get_selected_curve()
        if isinstance(curve, ErrorbarContainer):
            self.view.enable_errorbars_tab()
            if not curve.lines[0]:  # Errorbars can be plotted without a joining line
                self.view.disable_line_tab()
                self.view.disable_marker_tab()
            else:
                self.view.enable_line_tab()
                self.view.enable_marker_tab()
        else:
            self.view.enable_line_tab()
            self.view.enable_marker_tab()
            self.view.disable_errorbars_tab()

    def get_selected_ax(self):
        """
        Get selected axes object from name in combo box.
        If not found return None.
        """
        try:
            return self.axes_names_dict[self.view.get_selected_ax_name()]
        except KeyError:
            return None

    def get_selected_ax_errorbars(self):
        """Get all errorbar containers in selected axes"""
        ax = self.get_selected_ax()
        return [cont for cont in ax.containers if isinstance(cont, ErrorbarContainer)]

    def get_selected_curve(self):
        """Get selected Line2D or ErrorbarContainer object"""
        return self.curve_names_dict[self.view.get_selected_curve_name()]

    def get_selected_curves_curve_properties(self):
        """Get top level properties from curve"""
        return CurveProperties.from_curve(self.get_selected_curve())

    def get_view_curve_properties(self):
        """Get top level properties from view"""
        return CurveProperties.from_view(self.view)

    def populate_curve_combo_box_and_update_view(self):
        """
        Populate curve combo box and update the view with the curve's
        properties.
        """
        if self.populate_select_curve_combo_box():
            self.update_view()

    def populate_select_axes_combo_box(self):
        """
        Add Axes names to 'select axes' combo box.
        Names are generated similary to in AxesTabWidgetPresenter
        """
        # Sort names by axes position
        names = []
        for name, ax in self.axes_names_dict.items():
            if curve_in_ax(ax):
                names.append(name)
        names = sorted(names, key=lambda x: x[x.rfind("("):])
        self.view.populate_select_axes_combo_box(names)

    def populate_select_curve_combo_box(self):
        """
        Add curves on selected axes to the 'select curves' combo box.
        Return False if there are no lines on the axes (this can occur
        when a user uses the "Remove Curve" button), else return True.
        """
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.select_curve_combo_box.clear()
        selected_ax = self.get_selected_ax()
        if not selected_ax:
            self.view.close()
            return False

        curve_names = []
        for errorbar_container in self.get_selected_ax_errorbars():
            self._set_curve_name(errorbar_container, curve_names)
        for line in self.get_selected_ax().get_lines():
            self._set_curve_name(line, curve_names)

        self.view.populate_select_curve_combo_box(curve_names)
        return True

    def _set_curve_name(self, curve, curve_names):
        label = self._get_curve_name(curve, len(curve_names))
        if label:
            curve_names.append(label)
            self.curve_names_dict[label] = curve

    @staticmethod
    def _get_curve_name(curve, idx=0):
        if curve.get_label():
            if 'nolegend' not in curve.get_label():
                return curve.get_label()
        else:
            return '_nolabel_{}'.format(idx)

    def remove_selected_curve(self):
        """
        Remove selected curve from figure and combobox. If there are no
        curves left on the axes remove that axes from the axes combo box
        """
        self.get_selected_curve().remove()
        ax = self.get_selected_ax()
        ax.figure.canvas.draw()
        with BlockQSignals(self.view.select_curve_combo_box):
            self.view.remove_select_curve_combo_box_selected_item()
            if self.view.select_curve_combo_box.count() == 0:
                self.view.remove_select_axes_combo_box_selected_item()
                if self.view.select_axes_combo_box.count() == 0:
                    self.close_tab()
                    return
        self.update_view()

    def set_curve_label(self, curve, label):
        """Set label on curve and update its entry in the combo box"""
        self.update_legend()
        curve.set_label(label)
        if label:
            self.view.set_selected_curve_selector_text(label)
            self.curve_names_dict[label] = curve

    def update_presenter_lines(self):
        """Update the line in the sub-tabs' presenters to selected curve"""
        curve = self.get_selected_curve()
        if isinstance(curve, ErrorbarContainer):
            if curve[0]:
                self.line_tab.set_line(curve[0])
                self.marker_tab.set_line(curve[0])
            self.errorbars_tab.set_line(curve)
        else:
            self.line_tab.set_line(curve)
            self.marker_tab.set_line(curve)

    def update_view(self):
        """Update the view with the selected curve's properties"""
        curve_props = self.get_selected_curves_curve_properties()
        # Set top level view fields
        self.view.set_curve_label(curve_props.label)
        self.view.set_hide_curve(curve_props.hide)
        # Update sub-tab view fields
        self.update_presenter_lines()
        self.line_tab.update_view()
        self.marker_tab.update_view()
        self.errorbars_tab.update_view()
        # Enable/disable tabs
        self.enable_tabs()

    def update_legend(self):
        curve = self.get_selected_curve()
        if isinstance(curve, ErrorbarContainer):
            ax = curve.get_children()[0].axes
        else:
            ax = curve.axes
        if ax.get_legend():
            ax.legend()
