#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import, division, print_function

from workbench.widgets.plotselector.model import PlotSelectorModel
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter
from workbench.widgets.plotselector.view import PlotSelectorView, Column

import os

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


class PlotSelectorPresenterTest(unittest.TestCase):

    def side_effect_plot_name(self, plot_number):
        if plot_number == 0:
            return "Plot1"
        if plot_number == 1:
            return "Plot2"
        if plot_number == 2:
            return "Plot3"
        if plot_number == 42:
            return "Graph99"
        return None

    def setUp(self):
        self.view = mock.Mock(spec=PlotSelectorView)
        self.view.get_filter_text = mock.Mock(return_value="")

        self.model = mock.Mock(spec=PlotSelectorModel)
        self.model.get_plot_list = mock.Mock(return_value=[0, 1, 2, 42])
        self.model.get_plot_name_from_number = mock.Mock(side_effect=self.side_effect_plot_name)

        self.presenter = PlotSelectorPresenter(None, self.view, self.model)
        self.presenter.widget = self.view
        self.presenter.model = self.model

        # Ignore calls during the setup
        self.view.reset_mock()
        self.model.reset_mock()

    def convert_list_to_calls(self, list_to_convert):
        call_list = []
        for item in list_to_convert:
            call_list.append(mock.call(item))
        return call_list

    # ------------------------ Plot Updates ------------------------

    def test_plot_list_update(self):
        self.presenter.update_plot_list()
        self.assertEqual(self.model.get_plot_list.call_count, 1)
        self.view.set_plot_list.assert_called_once_with([0, 1, 2, 42])

    def test_append_to_plot_list_calls_update_in_model_and_view(self):
        self.presenter.append_to_plot_list(42)
        self.view.append_to_plot_list.assert_called_once_with(42)

    def test_remove_from_plot_list_calls_update_in_model_and_view(self):
        self.presenter.remove_from_plot_list(42)
        self.view.remove_from_plot_list.assert_called_once_with(42)

    def test_rename_in_plot_list_calls_update_in_model_and_view(self):
        self.presenter.rename_in_plot_list(42, "NewName")
        self.view.rename_in_plot_list.assert_called_once_with(42, "NewName")

    # ----------------------- Plot Filtering ------------------------

    def test_no_filtering_displays_all_plots(self):
        self.presenter.filter_text_changed()
        self.view.unhide_all_plots.assert_called_once_with()

    def filtering_calls_filter_on_view(self):
        self.view.get_filter_text = mock.Mock(return_value="Plot1")

        self.presenter.filter_text_changed()
        self.view.filter_plot_list.assert_called_once_with("Plot1")

    def test_plots_filtered_on_full_name(self):
        self.view.get_filter_text = mock.Mock(return_value="Plot1")
        self.assertTrue(self.presenter.is_shown_by_filter(0))  # Plot1
        self.assertFalse(self.presenter.is_shown_by_filter(1))  # Plot 2

    def test_plots_filtered_on_substring(self):
        self.view.get_filter_text = mock.Mock(return_value="lot")
        self.assertTrue(self.presenter.is_shown_by_filter(0))  # Plot1
        self.assertFalse(self.presenter.is_shown_by_filter(42))  # Graph99

    def test_filtering_case_invariant(self):
        self.view.get_filter_text = mock.Mock(return_value="pLOT1")
        self.assertTrue(self.presenter.is_shown_by_filter(0))  # Plot1
        self.assertFalse(self.presenter.is_shown_by_filter(1))  # Plot2

    # ------------------------ Plot Showing ------------------------

    def test_show_single_plot_shows_it(self):
        self.view.get_currently_selected_plot_number = mock.Mock(return_value=1)
        self.presenter.show_single_selected()
        self.model.show_plot.assert_called_once_with(1)

    def test_show_multiple_plots_shows_them(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[1, 2])
        self.presenter.show_multiple_selected()
        self.assertEqual(self.model.show_plot.mock_calls[0], mock.call(1))
        self.assertEqual(self.model.show_plot.mock_calls[1], mock.call(2))

    # ------------------------ Plot Renaming ------------------------

    def test_rename_figure_calls_rename_in_model(self):
        self.presenter.rename_figure(0, "NewName")
        self.model.rename_figure.assert_called_once_with(0, "NewName")

    def test_rename_figure_raising_a_value_error_undoes_rename_in_view(self):
        self.model.rename_figure.side_effect = ValueError("Some problem")
        self.presenter.rename_figure(0, "NewName")
        self.model.rename_figure.assert_called_once_with(0, "NewName")
        self.view.rename_in_plot_list.assert_called_once_with(0, "NewName")

    # ------------------------ Plot Closing -------------------------

    def test_close_action_single_plot(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[42])

        self.presenter.close_action_called()
        self.model.close_plot.assert_called_once_with(42)

    def test_close_action_multiple_plots(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[42, 43])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 2)
        self.model.close_plot.assert_has_calls(self.convert_list_to_calls([42, 43]),
                                               any_order=True)

    def test_close_action_with_model_call_raising_value_error(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[42])
        self.model.close_plot.side_effect = ValueError("Some problem")

        self.presenter.close_action_called()
        self.model.close_plot.assert_called_once_with(42)

    def test_close_action_with_no_plots_open(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[])
        self.model.configure_mock(plot_list=[])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 0)

    def test_close_action_with_no_plots_selected(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[])

        self.presenter.close_action_called()
        self.assertEqual(self.model.close_plot.call_count, 0)

    def test_close_single_plot_called(self):
        self.presenter.close_single_plot("Plot2")
        self.model.close_plot.assert_called_once_with("Plot2")

    # ----------------------- Plot Sorting --------------------------

    def test_set_sort_order_to_ascending_calls_view_update(self):
        self.presenter.set_sort_order(is_ascending=True)
        self.view.set_sort_order.assert_called_once_with(True)

    def test_set_sort_order_to_descending_calls_view_update(self):
        self.presenter.set_sort_order(is_ascending=False)
        self.view.set_sort_order.assert_called_once_with(False)

    def test_set_sort_type_to_name(self):
        self.view.sort_type = Column.Name
        self.presenter.set_sort_type(Column.Name)
        self.view.set_sort_type.assert_called_once_with(Column.Name)
        self.model.last_active_values.assert_not_called()
        self.view.set_last_active_values.assert_not_called()

    def test_set_sort_type_to_last_active(self):
        self.model.last_active_values = mock.Mock(return_value={0: 1, 1: 2})
        self.view.sort_type = Column.LastActive
        self.presenter.set_sort_type(Column.LastActive)

        self.view.set_sort_type.assert_called_once_with(Column.LastActive)
        self.view.set_last_active_values.assert_called_once_with({0: 1,
                                                                  1: 2})

    def test_update_last_active_values_with_sorting_by_last_active(self):
        self.model.last_active_values = mock.Mock(return_value={0: 1, 1: 2})
        self.view.sort_type = Column.LastActive
        self.presenter.update_last_active_order()

        self.view.set_last_active_values.assert_called_once_with({0: 1,
                                                                  1: 2})

    def test_update_last_active_values_with_sorting_by_name_does_nothing(self):
        self.model.last_active_order = mock.Mock(return_value={0: 1, 1: 2})
        self.view.sort_type = Column.Name
        self.presenter.update_last_active_order()

        self.model.last_active_order.assert_not_called()
        self.view.set_last_active_values.assert_not_called()

    def test_get_initial_last_active_value(self):
        self.assertEqual(self.presenter.get_initial_last_active_value(0), "_Plot1")

    def test_get_renamed_last_active_value_for_numeric_old_value(self):
        self.assertEqual(self.presenter.get_renamed_last_active_value(1, "23"), "23")

    def test_get_renamed_last_active_value_for_never_shown_value(self):
        self.assertEqual(self.presenter.get_renamed_last_active_value(0, "_Plot1"), "_Plot1")

    # ---------------------- Plot Exporting -------------------------

    def test_exporting_single_plot_generates_correct_filename(self):
        self.presenter.export_plot(0, '/home/Documents', '.xyz')
        self.model.export_plot.assert_called_once_with(0, '/home/Documents' + os.sep + 'Plot1.xyz')

    def test_exporting_multiple_plots_generates_correct_filename(self):
        self.view.get_all_selected_plot_numbers = mock.Mock(return_value=[0, 1, 2])
        self.presenter.export_plots('/home/Documents', '.xyz')
        for i in range(len(self.model.export_plot.mock_calls)):
            self.assertEqual(self.model.export_plot.mock_calls[i],
                             mock.call(i, '/home/Documents' + os.sep + 'Plot{}.xyz'.format(i+1)))


if __name__ == '__main__':
    unittest.main()
