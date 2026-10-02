// European IEEE9 power flow: Bisseling et al., WIW 2024,
// DOI 10.1049/icp.2024.3847, Tables 1/2 and Figure 2.
// Static PQ-load / pi-line variant, not the paper's EMT model.

#include <DPsim.h>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
using namespace CPS;
using namespace DPsim;

int main() {
  const String simName = "SP_Ph1_EuIEEE9_PF";
  Logger::setLogDir("logs/" + simName);

  constexpr Real fNom = 50.0; // Nominal frequency [Hz]
  constexpr Real omegaNom =
      2.0 * PI * fNom;         // Nominal angular frequency [rad/s]
  constexpr Real vNom = 400e3; // Nominal line-to-line voltage [V]

  auto bus1 = SimNode<Complex>::make("Bus1", PhaseType::Single);
  auto bus2 = SimNode<Complex>::make("Bus2", PhaseType::Single);
  auto bus3 = SimNode<Complex>::make("Bus3", PhaseType::Single);

  auto bus4 = SimNode<Complex>::make("Bus4", PhaseType::Single);
  auto bus5 = SimNode<Complex>::make("Bus5", PhaseType::Single);
  auto bus6 = SimNode<Complex>::make("Bus6", PhaseType::Single);

  auto bus7 = SimNode<Complex>::make("Bus7", PhaseType::Single);
  auto bus8 = SimNode<Complex>::make("Bus8", PhaseType::Single);
  auto bus9 = SimNode<Complex>::make("Bus9", PhaseType::Single);

  // Static VD/PV generators. All powers are total three-phase values.
  // Gen1 is the slack: its P and Q are results; 361 MW is a reference only.
  // Generators:
  auto gen1 = SP::Ph1::SynchronGenerator::make("Gen1", Logger::Level::off);
  gen1->setParameters(600e6, 21e3, 361e6, 1.08 * 21e3, PowerflowBusType::VD);
  gen1->setBaseVoltage(21e3);
  gen1->connect({bus1});

  auto gen2 = SP::Ph1::SynchronGenerator::make("Gen2", Logger::Level::off);
  gen2->setParameters(1200e6, 27e3, 908e6, 1.075 * 27e3, PowerflowBusType::PV);
  gen2->setBaseVoltage(27e3);
  gen2->connect({bus2});

  auto gen3 = SP::Ph1::SynchronGenerator::make("Gen3", Logger::Level::off);
  gen3->setParameters(600e6, 21e3, 471e6, 1.049 * 21e3, PowerflowBusType::PV);
  gen3->setBaseVoltage(21e3);
  gen3->connect({bus3});

  // Positive P/Q denotes consumption. These loads are constant PQ;
  // the paper uses constant-impedance loads in its transient studies.
  // Loads:
  auto load5 = SP::Ph1::Load::make("Load5", Logger::Level::off);
  load5->setParameters(687.5e6, 275e6, vNom);
  load5->modifyPowerFlowBusType(PowerflowBusType::PQ);
  load5->connect({bus5});

  auto load6 = SP::Ph1::Load::make("Load6", Logger::Level::off);
  load6->setParameters(495e6, 165e6, vNom);
  load6->modifyPowerFlowBusType(PowerflowBusType::PQ);
  load6->connect({bus6});

  auto load8 = SP::Ph1::Load::make("Load8", Logger::Level::off);
  load8->setParameters(550e6, 192.5e6, vNom);
  load8->modifyPowerFlowBusType(PowerflowBusType::PQ);
  load8->connect({bus8});

  // PROVISIONAL line data: the paper does not tabulate numerical R/L/C.
  // Open substitute: PyPSA v0.30.2, "Al/St 240/40 4-bundle 380.0":
  // https://docs.pypsa.org/v0.30.2/user-guide/components.html#line-types
  // Replace these values when the paper's original line data is available.
  constexpr Real R_per_km = 0.03;    // [ohm/km]
  constexpr Real X_per_km = 0.246;   // [ohm/km] at 50 Hz
  constexpr Real C_per_km = 13.8e-9; // total shunt capacitance [F/km]
  constexpr Real G_per_km = 0.0;     // [S/km], neglected leakage
  constexpr Real L_per_km = X_per_km / omegaNom; // [H/km]

  auto line45 = SP::Ph1::PiLine::make("Line45", Logger::Level::off);
  line45->setParameters(R_per_km * 25.0, L_per_km * 25.0, C_per_km * 25.0,
                        G_per_km * 25.0);
  line45->setBaseVoltage(vNom);

  auto line46 = SP::Ph1::PiLine::make("Line46", Logger::Level::off);
  line46->setParameters(R_per_km * 30.0, L_per_km * 30.0, C_per_km * 30.0,
                        G_per_km * 30.0);
  line46->setBaseVoltage(vNom);

  auto line57 = SP::Ph1::PiLine::make("Line57", Logger::Level::off);
  line57->setParameters(R_per_km * 35.0, L_per_km * 35.0, C_per_km * 35.0,
                        G_per_km * 35.0);
  line57->setBaseVoltage(vNom);

  auto line78 = SP::Ph1::PiLine::make("Line78", Logger::Level::off);
  line78->setParameters(R_per_km * 40.0, L_per_km * 40.0, C_per_km * 40.0,
                        G_per_km * 40.0);
  line78->setBaseVoltage(vNom);

  auto line89 = SP::Ph1::PiLine::make("Line89", Logger::Level::off);
  line89->setParameters(R_per_km * 50.0, L_per_km * 50.0, C_per_km * 50.0,
                        G_per_km * 50.0);
  line89->setBaseVoltage(vNom);

  auto line96 = SP::Ph1::PiLine::make("Line96", Logger::Level::off);
  line96->setParameters(R_per_km * 50.0, L_per_km * 50.0, C_per_km * 50.0,
                        G_per_km * 50.0);
  line96->setBaseVoltage(vNom);

  line45->connect({bus4, bus5});
  line46->connect({bus4, bus6});
  line57->connect({bus5, bus7});
  line78->connect({bus7, bus8});
  line89->connect({bus8, bus9});
  line96->connect({bus9, bus6});

  // Transformers: interpret Table 2's nominal copper losses as R = 0.015 pu;
  // X = 0.20 pu, each on its own transformer rating, referred to HV.
  // End 0 is LV here: ratio = V_LV / V_HV, with LV leading by 30 deg (YNd11).
  // The PF stamp also supports this LV-first ordering without MNA initialization.
  Real Zbase600 = vNom * vNom / 600e6;
  Real R600 = 0.015 * Zbase600;
  Real X600 = 0.20 * Zbase600;
  Real L600 = X600 / omegaNom;

  Real Zbase1200 = vNom * vNom / 1200e6;
  Real R1200 = 0.015 * Zbase1200;
  Real X1200 = 0.20 * Zbase1200;
  Real L1200 = X1200 / omegaNom;

  auto trafo14 = SP::Ph1::Transformer::make("Trafo14", Logger::Level::off);
  trafo14->setParameters(21e3, vNom, 600e6, 21e3 / vNom, PI / 6.0, R600, L600);
  trafo14->setBaseVoltage(vNom);

  auto trafo27 = SP::Ph1::Transformer::make("Trafo27", Logger::Level::off);
  trafo27->setParameters(27e3, vNom, 1200e6, 27e3 / vNom, PI / 6.0, R1200,
                         L1200);
  trafo27->setBaseVoltage(vNom);

  auto trafo39 = SP::Ph1::Transformer::make("Trafo39", Logger::Level::off);
  trafo39->setParameters(21e3, vNom, 600e6, 21e3 / vNom, PI / 6.0, R600, L600);
  trafo39->setBaseVoltage(vNom);

  trafo14->connect({bus1, bus4});
  trafo27->connect({bus2, bus7});
  trafo39->connect({bus3, bus9});

  // System topology:
  auto systemPF = SystemTopology(
      fNom,
      SystemNodeList{bus1, bus2, bus3, bus4, bus5, bus6, bus7, bus8, bus9},
      SystemComponentList{gen1, gen2, gen3, load5, load6, load8, line45, line46,
                          line57, line78, line89, line96, trafo14, trafo27,
                          trafo39});
  const std::array<SimNode<Complex>::Ptr, 9> buses = {
      bus1, bus2, bus3, bus4, bus5, bus6, bus7, bus8, bus9};
  const std::array<Real, 9> baseVoltages = {21e3, 27e3, 21e3, vNom, vNom,
                                            vNom, vNom, vNom, vNom};
  const auto generators = {gen1, gen2, gen3};
  auto logger = DataLogger::make(simName);
  for (size_t i = 0; i < buses.size(); ++i)
    logger->logAttribute("v_bus" + std::to_string(i + 1),
                         buses[i]->attribute("v"));
  for (const auto &gen : generators) {
    logger->logAttribute(gen->name() + "_P", gen->attribute("P_set"));
    logger->logAttribute(gen->name() + "_Q", gen->attribute("Q_set"));
  }
  // Log the actual PQ inputs as well, so notebook comparisons remain valid
  // when the load set points in this example are changed.
  for (const auto &load : {load5, load6, load8}) {
    logger->logAttribute(load->name() + "_P", load->attribute("P"));
    logger->logAttribute(load->name() + "_Q", load->attribute("Q"));
  }
  Simulation sim(simName, Logger::Level::info);
  sim.setSystem(systemPF);
  sim.setDomain(Domain::SP);
  sim.setSolverType(Solver::Type::NRP);

  sim.setTimeStep(1.0);
  sim.setFinalTime(1.0); // one stationary operating point
  sim.setSolverAndComponentBehaviour(Solver::Behaviour::Simulation);
  sim.doInitFromNodesAndTerminals(false);
  sim.addLogger(logger);
  sim.run();
