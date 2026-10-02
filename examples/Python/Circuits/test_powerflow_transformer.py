"""Check SP transformer phase shifts against a two-bus analytical solution.

Run with the freshly built dpsimpy on PYTHONPATH:
    PYTHONPATH=build python examples/Python/Circuits/test_powerflow_transformer.py
"""

import cmath
import math
from itertools import product
import tempfile
import unittest

import dpsimpy


class TransformerPowerFlowTest(unittest.TestCase):
    def test_phase_shift_and_terminal_order(self):
        # A fixed LV source feeds a PQ load through HV-referred R+jX.
        # Solve the receiving-end voltage analytically from
        # E = V + Z * conj(S/V), using line-line RMS and three-phase S.
        lv, hv, rating = 21e3, 400e3, 600e6
        z = complex(0.015, 0.20) * hv**2 / rating
        previous_log_dir = dpsimpy.Logger.get_log_dir()
        try:
            with tempfile.TemporaryDirectory(prefix="dpsim-trafo-pf-") as log_dir:
                dpsimpy.Logger.set_log_dir(log_dir)
                cases = product(
                    (0j, complex(300e6, 100e6)),
                    (0.0, math.pi / 6, -math.pi / 6),
                    (1.0, 0.97),
                    (False, True),
                    (False, True),
                )
                for load_power, angle, tap_pu, lv_first, initialize in cases:
                    with self.subTest(
                        load=load_power,
                        angle=angle,
                        tap=tap_pu,
                        lv_first=lv_first,
                        initialize=initialize,
                    ):
                        name = f"case_{load_power.real}_{angle}_{tap_pu}_{lv_first}_{initialize}"
                        n_lv = dpsimpy.sp.SimNode("lv", dpsimpy.PhaseType.Single)
                        n_hv = dpsimpy.sp.SimNode("hv", dpsimpy.PhaseType.Single)
                        source = dpsimpy.sp.ph1.SynchronGenerator("source")
                        source.set_parameters(
                            rating, lv, load_power.real, lv, dpsimpy.PowerflowBusType.VD
                        )
                        source.set_base_voltage(lv)
                        source.connect([n_lv])
                        load = dpsimpy.sp.ph1.Load("load")
                        load.set_parameters(load_power.real, load_power.imag, hv)
                        load.modify_power_flow_bus_type(dpsimpy.PowerflowBusType.PQ)
                        load.connect([n_hv])
                        trafo = dpsimpy.sp.ph1.Transformer("trafo")
                        ratio = hv / lv * tap_pu
                        if lv_first:
                            trafo.set_parameters(
                                lv,
                                hv,
                                rating,
                                1 / ratio,
                                -angle,
                                z.real,
                                z.imag / (2 * math.pi * 50),
                            )
                            trafo.connect([n_lv, n_hv])
                        else:
                            trafo.set_parameters(
                                hv,
                                lv,
                                rating,
                                ratio,
                                angle,
                                z.real,
                                z.imag / (2 * math.pi * 50),
                            )
                            trafo.connect([n_hv, n_lv])
                        trafo.set_base_voltage(hv)
                        system = dpsimpy.SystemTopology(
                            50, [n_lv, n_hv], [source, load, trafo]
                        )
                        sim = dpsimpy.Simulation(name)
                        sim.set_system(system)
                        sim.set_domain(dpsimpy.Domain.SP)
                        sim.set_solver(dpsimpy.Solver.NRP)
                        sim.set_solver_component_behaviour(
                            dpsimpy.SolverBehaviour.Simulation
                        )
                        sim.do_init_from_nodes_and_terminals(initialize)
                        sim.set_time_step(1)
                        sim.set_final_time(1)
                        sim.run()

                        ideal_hv = cmath.rect(hv * tap_pu, angle)
                        a = abs(ideal_hv) ** 2 - 2 * (z * load_power.conjugate()).real
                        voltage_sq = (
                            a + math.sqrt(a * a - 4 * abs(z * load_power) ** 2)
                        ) / 2
                        expected_voltage = (
                            voltage_sq + z.conjugate() * load_power
                        ) / ideal_hv.conjugate()
                        expected_power = (
                            load_power + z * abs(load_power) ** 2 / voltage_sq
                        )
                        self.assertLess(
                            abs(n_hv.single_voltage() - expected_voltage), 0.01
                        )
                        self.assertLess(abs(n_lv.single_voltage() - lv), 0.01)
                        # This also catches missing conjugation of the tap,
                        # which violates complex-power conservation.
                        self.assertLess(
                            abs(source.get_apparent_power() - expected_power), 100
                        )
        finally:
            dpsimpy.Logger.set_log_dir(previous_log_dir)


if __name__ == "__main__":
    unittest.main()
