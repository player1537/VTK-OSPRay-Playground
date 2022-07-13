/**
 *
 */

// stdlib
#include <array>
#include <complex>
#include <cstdint>
#include <vector>

// vtk
#include <vtkCellData.h>
#include <vtkDataObject.h>
#include <vtkHexahedron.h>
#include <vtkMPIController.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPDistributedDataFilter.h>
#include <vtkPKdTree.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
#include <vtkUnsignedShortArray.h>
#include <vtkUnstructuredGrid.h>

// OSPRay
#include <ospray/ospray.h>
#include <ospray/ospray_util.h>


//---

struct Mandelbrot {
  using ScalarF = float;
  using ScalarU = uint16_t;
  using BoundsF = std::array<ScalarF, 6>;
  enum Bounds { MinX = 0, MinY, MinZ, MaxX, MaxY, MaxZ };
  using ComplexF = std::complex<ScalarF>;

  enum Debug { OnlyData, OnlyNsteps };

  Mandelbrot() = default;
  Mandelbrot(Mandelbrot &) = delete;
  Mandelbrot(Mandelbrot &&) = default;
  Mandelbrot(size_t nx_, size_t ny_, size_t nz_, BoundsF bounds_);
  Mandelbrot &operator=(Mandelbrot &) = delete;
  ~Mandelbrot() = default;

  void debug(Debug);
  void step(size_t dt);
  vtkUnstructuredGrid *vtk(vtkUnstructuredGrid *unstructuredGrid=nullptr);

  size_t nx{0}, ny{0}, nz{0};
  BoundsF bounds{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  std::vector<ScalarF> data{};
  std::vector<ScalarU> nsteps{};
};

Mandelbrot::Mandelbrot(size_t nx_, size_t ny_, size_t nz_, Mandelbrot::BoundsF bounds_)
  : nx(nx_)
  , ny(ny_)
  , nz(nz_)
  , bounds(bounds_)
  , data(2*nx_*ny_*nz_)
  , nsteps(nx_*ny_*nz_)
{
  for (size_t zi=0; zi<nz; ++zi) {
    size_t zindex = zi*ny*nx;

    for (size_t yi=0; yi<ny; ++yi) {
      size_t yindex = zindex + yi*nx;

      for (size_t xi=0; xi<nx; ++xi) {
        size_t xindex = yindex + xi;

        data[2*xindex+0] = 0.0f;
        data[2*xindex+1] = 0.0f;
        nsteps[xindex] = 0;
      }
    }
  }
}

void Mandelbrot::debug(Debug which) {
  for (size_t zi=0; zi<nz; ++zi) {
    size_t zindex = zi*ny*nx;

    std::fprintf(stderr, "[");

    for (size_t yi=0; yi<ny; ++yi) {
      size_t yindex = zindex + yi*nx;

      if (yi == 0) std::fprintf(stderr, " [");
      else std::fprintf(stderr, "  [");

      for (size_t xi=0; xi<nx; ++xi) {
        size_t xindex = yindex + xi;

        if (which == OnlyData) {
          std::fprintf(stderr, " %+0.2f%+0.2fi", data[2*xindex+0], data[2*xindex+1]);
        } else if (which == OnlyNsteps) {
          std::fprintf(stderr, " %03d", nsteps[xindex]);
        }
      }
      
      std::fprintf(stderr, "\n");
    }

    std::fprintf(stderr, "\n");
  }
}

void Mandelbrot::step(size_t dt) {
  for (size_t zi=0; zi<nz; ++zi) {
    ScalarF zratio = (ScalarF)zi / (ScalarF)nz;
    ScalarF z = std::get<MinZ>(bounds) * zratio + std::get<MaxZ>(bounds) * (1.0f - zratio);
    size_t zindex = zi*ny*nx;

    for (size_t yi=0; yi<ny; ++yi) {
      ScalarF yratio = (ScalarF)yi / (ScalarF)ny;
      ScalarF y = std::get<MinY>(bounds) * yratio + std::get<MaxY>(bounds) * (1.0f - yratio);
      size_t yindex = zindex + yi*nx;

      for (size_t xi=0; xi<nx; ++xi) {
        ScalarF xratio = (ScalarF)xi / (ScalarF)nx;
        ScalarF x = std::get<MinX>(bounds) * xratio + std::get<MaxX>(bounds) * (1.0f - xratio);
        size_t xindex = yindex + xi;

        for (size_t ti=0; ti<dt; ++ti) {
          ScalarF xd = data[2*xindex+0];
          ScalarF yd = data[2*xindex+1];

          if (xd*xd + yd*yd >= 2.0) {
            break;
          }

          ComplexF temp = std::pow(ComplexF(xd, yd), z);
          data[2*xindex+0] = temp.real() + x;
          data[2*xindex+1] = temp.imag() + y;
          ++nsteps[xindex];
        }
      }
    }
  }
}

vtkUnstructuredGrid *Mandelbrot::vtk(vtkUnstructuredGrid *unstructuredGrid) {
  using Points = vtkPoints;
  using Array = vtkUnsignedShortArray;

  Points *points;
  Array *array;

  if (unstructuredGrid == nullptr) {
    points = Points::New();

    array = Array::New();
    array->SetName("nsteps");

    unstructuredGrid = vtkUnstructuredGrid::New();
    unstructuredGrid->EditableOn();
    unstructuredGrid->GetCellData()->AddArray(array);
    unstructuredGrid->SetPoints(points);

  } else {
    points = unstructuredGrid->GetPoints();

    array = Array::SafeDownCast(unstructuredGrid->GetCellData()->GetAbstractArray("nsteps"));
  }

  using IdType = vtkIdType;

  using Cell = vtkHexahedron;
  vtkNew<Cell> cell;

  for (size_t i=0, zi=0; zi<nz; ++zi) {
    ScalarF z0ratio = (ScalarF)(zi + 0) / (ScalarF)nz;
    ScalarF z0 = std::get<MinZ>(bounds) * z0ratio + std::get<MaxZ>(bounds) * (1.0f - z0ratio);
    ScalarF z1ratio = (ScalarF)(zi + 1) / (ScalarF)nz;
    ScalarF z1 = std::get<MinZ>(bounds) * z1ratio + std::get<MaxZ>(bounds) * (1.0f - z1ratio);
    size_t zindex = zi*ny*nx;

    for (size_t yi=0; yi<ny; ++yi) {
      ScalarF y0ratio = (ScalarF)(yi + 0) / (ScalarF)ny;
      ScalarF y0 = std::get<MinY>(bounds) * y0ratio + std::get<MaxY>(bounds) * (1.0f - y0ratio);
      ScalarF y1ratio = (ScalarF)(yi + 1) / (ScalarF)ny;
      ScalarF y1 = std::get<MinY>(bounds) * y1ratio + std::get<MaxY>(bounds) * (1.0f - y1ratio);
      size_t yindex = zindex + yi*nx;

      for (size_t xi=0; xi<nx; ++xi, ++i) {
        ScalarF x0ratio = (ScalarF)(xi + 0) / (ScalarF)nx;
        ScalarF x0 = std::get<MinX>(bounds) * x0ratio + std::get<MaxX>(bounds) * (1.0f - x0ratio);
        ScalarF x1ratio = (ScalarF)(xi + 1) / (ScalarF)nx;
        ScalarF x1 = std::get<MinX>(bounds) * x1ratio + std::get<MaxX>(bounds) * (1.0f - x1ratio);
        size_t xindex = yindex + xi;

        cell->GetPointIds()->SetId(0, points->InsertNextPoint(x0, y0, z0));
        cell->GetPointIds()->SetId(1, points->InsertNextPoint(x1, y0, z0));
        cell->GetPointIds()->SetId(2, points->InsertNextPoint(x1, y1, z0));
        cell->GetPointIds()->SetId(3, points->InsertNextPoint(x0, y1, z0));
        cell->GetPointIds()->SetId(4, points->InsertNextPoint(x0, y0, z1));
        cell->GetPointIds()->SetId(5, points->InsertNextPoint(x1, y0, z1));
        cell->GetPointIds()->SetId(6, points->InsertNextPoint(x1, y1, z1));
        cell->GetPointIds()->SetId(7, points->InsertNextPoint(x0, y1, z1));

        array->InsertNextValue(nsteps[xindex]);

        unstructuredGrid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
      }
    }
  }

  return unstructuredGrid;
}


//---

struct Assignment {
  Assignment() = default;
  ~Assignment() = default;

  size_t rank{0};
  size_t xindex{0};
  size_t yindex{0};
  size_t zindex{0};
};


//---

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  using Controller = vtkMPIController;
  vtkNew<Controller> controller;
  controller->Initialize(&argc, &argv, /* initializedExternally= */0);
  struct guard {
    guard(Controller *c) { vtkMultiProcessController::SetGlobalController(c); };
    ~guard() { vtkMultiProcessController::GetGlobalController()->Finalize(); };
  } guard(controller);

#define DEBUG(Msg)                                                             \
  do {                                                                         \
    for (size_t _DEBUG_i=0; _DEBUG_i<opt_nprocs; ++_DEBUG_i) {                 \
      if (controller->Barrier(), _DEBUG_i == opt_rank) {                       \
        std::cout << opt_rank << ": " Msg << std::endl << std::flush;          \
      }                                                                        \
    }                                                                          \
  } while (0)

#define DEBUG_RANK0(Msg)                                                       \
  do {                                                                         \
    if (controller->Barrier(), opt_rank == 0) {                                \
      std::cout Msg << std::endl << std::flush;                                \
    }                                                                          \
  } while (0)

  size_t opt_rank;
  size_t opt_nprocs;
  size_t opt_nx;
  size_t opt_ny;
  size_t opt_nz;
  size_t opt_nxcuts;
  size_t opt_nycuts;
  size_t opt_nzcuts;
  size_t opt_nsteps;
  float opt_xmin;
  float opt_ymin;
  float opt_zmin;
  float opt_xmax;
  float opt_ymax;
  float opt_zmax;
  bool opt_enable_d3;

  opt_rank = controller->GetLocalProcessId();
  opt_nprocs = controller->GetNumberOfProcesses();
  opt_nx = 16;
  opt_ny = 16;
  opt_nz = 16;
  opt_nxcuts = 4;
  opt_nycuts = 4;
  opt_nzcuts = 4;
  opt_nsteps = 16;
  opt_xmin = -2.0f;
  opt_ymin = -2.0f;
  opt_zmin = 2.0f;
  opt_xmax = +2.0f;
  opt_ymax = +2.0f;
  opt_zmax = 4.0f;
  opt_enable_d3 = false;

#define ARGLOOP \
  if (char *ARGVAL=nullptr) \
    ; \
  else \
    for (int ARGIND=1; ARGIND<argc; ARGVAL=NULL, ++ARGIND) \
      if (0) \
        ;

#define ARG(s) \
      else if (strncmp(argv[ARGIND], s, sizeof(s)) == 0 && ++ARGIND < argc && (ARGVAL = argv[ARGIND], 1))

  ARGLOOP
  ARG("-rank") opt_rank = (size_t)std::stoull(ARGVAL);
  ARG("-nprocs") opt_nprocs = (size_t)std::stoull(ARGVAL);
  ARG("-nx") opt_nx = (size_t)std::stoull(ARGVAL);
  ARG("-ny") opt_ny = (size_t)std::stoull(ARGVAL);
  ARG("-nz") opt_nz = (size_t)std::stoull(ARGVAL);
  ARG("-nxcuts") opt_nxcuts = (size_t)std::stoull(ARGVAL);
  ARG("-nycuts") opt_nycuts = (size_t)std::stoull(ARGVAL);
  ARG("-nzcuts") opt_nzcuts = (size_t)std::stoull(ARGVAL);
  ARG("-nsteps") opt_nsteps = (size_t)std::stoull(ARGVAL);
  ARG("-xmin") opt_xmin = std::stof(ARGVAL);
  ARG("-ymin") opt_ymin = std::stof(ARGVAL);
  ARG("-zmin") opt_zmin = std::stof(ARGVAL);
  ARG("-xmax") opt_xmax = std::stof(ARGVAL);
  ARG("-ymax") opt_ymax = std::stof(ARGVAL);
  ARG("-zmax") opt_zmax = std::stof(ARGVAL);
  ARG("-d3") opt_enable_d3 = (bool)std::stoi(ARGVAL);

#undef ARG
#undef ARGLOOP

  std::vector<Assignment> assignments;
  for (size_t i=0, xi=0; xi<opt_nxcuts; ++xi) {
    for (size_t yi=0; yi<opt_nycuts; ++yi) {
      for (size_t zi=0; zi<opt_nzcuts; ++zi, ++i) {
        assignments.emplace_back(std::move(Assignment{i % opt_nprocs, xi, yi, zi}));
      }
    }
  }

  std::vector<Mandelbrot> mandelbrots;
  for (size_t i=0; i<assignments.size(); ++i) {
    if (assignments[i].rank == opt_rank) {
      mandelbrots.emplace_back(opt_nx, opt_ny, opt_nz, Mandelbrot::BoundsF({
        opt_xmin + (opt_xmax - opt_xmin) / opt_nxcuts * (assignments[i].xindex + 0),
        opt_ymin + (opt_ymax - opt_ymin) / opt_nycuts * (assignments[i].yindex + 0),
        opt_zmin + (opt_zmax - opt_zmin) / opt_nzcuts * (assignments[i].zindex + 0),
        opt_xmin + (opt_xmax - opt_xmin) / opt_nxcuts * (assignments[i].xindex + 1),
        opt_ymin + (opt_ymax - opt_ymin) / opt_nycuts * (assignments[i].yindex + 1),
        opt_zmin + (opt_zmax - opt_zmin) / opt_nzcuts * (assignments[i].zindex + 1),
      }));
    }
  }

  for (size_t i=0; i<mandelbrots.size(); ++i) {
    mandelbrots[i].step(opt_nsteps);
  }

  using UnstructuredGrid = vtkUnstructuredGrid;
  vtkSmartPointer<UnstructuredGrid> unstructuredGrid = nullptr;
  for (size_t i=0; i<mandelbrots.size(); ++i) {
    unstructuredGrid = mandelbrots[i].vtk(unstructuredGrid);
  }

  unstructuredGrid->GetCellData()->SetActiveScalars("nsteps");

  DEBUG(<< "opt_enable_d3: " << opt_enable_d3);
  if (opt_enable_d3) {
    using TimerLog = vtkTimerLog;
    TimerLog::SetMaxEntries(2048);

    using DistributedDataFilter = vtkPDistributedDataFilter;
    vtkNew<DistributedDataFilter> distributedDataFilter;
    distributedDataFilter->GetKdtree()->AssignRegionsRoundRobin();
    distributedDataFilter->SetInputData(unstructuredGrid);
    distributedDataFilter->SetBoundaryMode(0);
    distributedDataFilter->SetUseMinimalMemory(1);
    distributedDataFilter->SetMinimumGhostLevel(0);
    distributedDataFilter->RetainKdtreeOn();
    DEBUG_RANK0(<< "D3: " << *distributedDataFilter);
    distributedDataFilter->Update();

    using KdTree = vtkPKdTree;
    vtkSmartPointer<KdTree> kdTree = distributedDataFilter->GetKdtree();

    DEBUG_RANK0(<< "kdTree: " << *kdTree);

    unstructuredGrid = UnstructuredGrid::SafeDownCast(distributedDataFilter->GetOutput());
  }

  DEBUG(<< *unstructuredGrid);

  // using CompositeDataIterator = vtkCompositeDataIterator;
  // vtkSmartPointer<CompositeDataIterator> compositeDataIterator = multiBlockDataSet->NewIterator();
  // compositeDataIterator->InitTraversal();
  // while (!compositeDataIterator->IsDoneWithTraversal()) {
  //   using DataObject = vtkDataObject;
  //   DataObject *dataObject = compositeDataIterator->GetCurrentDataObject();

  //   std::cout << *dataObject << std::endl;

  //   compositeDataIterator->GoToNextItem();
  // }

  int width = 512;
  int height = 512;
  OSPDevice device{nullptr};
  OSPData volumeDataData{nullptr};
  OSPVolume volume{nullptr};
  std::vector<float> transferFunctionColor{};
  OSPData transferFunctionColorData{nullptr};
  std::vector<float> TransferFunctionOpacity{};
  OSPData transferFunctionOpacityData{nullptr};
  OSPTransferFunction transferFunction{nullptr};
  OSPVolumetricModel VolumetricModel{nullptr};
  OSPGroup group{nullptr};
  OSPInstance instance{nullptr};
  OSPLight light{nullptr};
  std::vector<float> worldRegion;
  OSPData worldRegionData{nullptr};
  OSPWorld world{nullptr};
  OSPCamera camera{nullptr};
  OSPRenderer renderer{nullptr};
  OSPFrameBuffer frameBuffer{nullptr};

  ospLoadModule("mpi");

  device = ospNewDevice("mpiDistributed");
  ospDeviceCommit(device);
  ospSetCurrentDevice(device);

  // if (volumeDataData) {
  //   ospRelease(volumeDataData);
  //   volumeDataData = nullptr;
  // }

  // volumeDataData = ospNewSharedData(volume, OSP_USHORT, nx, 0, ny, 0, nz, 0);
  // ospCommit(volumeDataData);

  // volume = ospNewVolume("structuredRegular");
  // ospSetVec3f(volume, "gridOrigin", -0.5f, -0.5f, -0.5f);
  // ospSetVec3f(volume, "gridSpacing", 1.0f/(float)nx, 1.0f/(float)ny, 1.0f/(float)nz);
  // ospSetObject(volume, "data", volumeDataData);
  // ospSetBool(volume, "cellCentered", 1);
  // ospCommit(volume);

  // if (transferFunctionColorData) {
  //   ospRelease(transferFunctionColorData);
  //   transferFunctionColorData = nullptr;
  // }

  // transferFunctionColor.clear();
  // transferFunctionColor.insert(transferFunctionColor.end(), {
  //   (CommRank % 3 == 0 ? 1.0f : 0.0f),
  //   (CommRank % 3 == 1 ? 1.0f : 0.0f),
  //   (CommRank % 3 == 2 ? 1.0f : 0.0f),
  //   (CommRank % 3 == 0 ? 1.0f : 0.0f),
  //   (CommRank % 3 == 1 ? 1.0f : 0.0f),
  //   (CommRank % 3 == 2 ? 1.0f : 0.0f),
  // });

  // transferFunctionColorData = ospNewSharedData(transferFunctionColor.data(), OSP_VEC3F, transferFunctionColor.size() / 3);
  // ospCommit(transferFunctionColorData);

  // if (transferFunctionOpacityData) {
  //   ospRelease(transferFunctionOpacityData);
  //   transferFunctionOpacityData = nullptr;
  // }

  // TransferFunctionOpacity.clear();
  // TransferFunctionOpacity.insert(TransferFunctionOpacity.end(), {
  //   0.0f,
  //   1.0f,
  // });

  // transferFunctionOpacityData = ospNewSharedData(TransferFunctionOpacity.data(), OSP_FLOAT, TransferFunctionOpacity.size() / 1);
  // ospCommit(transferFunctionOpacityData);

  // transferFunction = ospNewTransferFunction("piecewiseLinear");
  // ospSetObject(transferFunction, "color", transferFunctionColorData);
  // ospSetObject(transferFunction, "opacity", transferFunctionOpacityData);
  // ospSetVec2f(transferFunction, "valueRange", (float)0.0f, (float)hi);
  // ospCommit(transferFunction);

  // VolumetricModel = ospNewVolumetricModel(nullptr);
  // ospSetObject(VolumetricModel, "volume", volume);
  // ospSetObject(VolumetricModel, "transferFunction", transferFunction);
  // ospCommit(VolumetricModel);

  group = ospNewGroup();
  ospSetObjectAsData(group, "volume", OSP_VOLUMETRIC_MODEL, VolumetricModel);
  ospCommit(group);

  instance = ospNewInstance(nullptr);
  ospSetObject(instance, "group", group);
  ospCommit(instance);

  light = ospNewLight("ambient");
  ospCommit(light);

  world = ospNewWorld();
  ospSetObjectAsData(world, "instance", OSP_INSTANCE, instance);
  ospSetObjectAsData(world, "light", OSP_LIGHT, light);
  ospSetObject(world, "region", worldRegionData);
  ospCommit(world);

  camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", (float)width / (float)height);
  ospSetVec3f(camera, "position", 0.0f, 0.0f, 0.75f);
  ospSetVec3f(camera, "direction", 0.0f, 0.0f, -1.0f);
  ospSetVec3f(camera, "up", 0.0f, 1.0f, 0.0f);
  ospCommit(camera);

  renderer = ospNewRenderer("mpiRaycast");
  ospSetInt(renderer, "pixelSamples", 16);
  ospSetVec3f(renderer, "backgroundColor", 0.0f, 0.0f, 0.0f);
  ospCommit(renderer);

  frameBuffer = ospNewFrameBuffer(width, height, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);

  return 0;
}
