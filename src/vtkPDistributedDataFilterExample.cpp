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
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkInformation.h>
#include <vtkMPIController.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPDistributedDataFilter.h>
#include <vtkPKdTree.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>
#include <vtkUnsignedShortArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkRenderWindow.h>
#include <vtkOSPRayPass.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkUnstructuredGridVolumeRayCastMapper.h>
#include <vtkVolume.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkWindowToImageFilter.h>
#include <vtkJPEGWriter.h>
#include <vtkObjectFactoryCollection.h>

// OSPRay
#include <ospray/ospray.h>
#include <ospray/ospray_util.h>

// MPI
#include <mpi.h>


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
  if (nx > 16 || ny > 16 || nz > 16) {
    return;
  }

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
    ScalarF z = std::get<MinZ>(bounds) + zratio * (std::get<MaxZ>(bounds) - std::get<MinZ>(bounds));
    size_t zindex = zi*ny*nx;

    for (size_t yi=0; yi<ny; ++yi) {
      ScalarF yratio = (ScalarF)yi / (ScalarF)ny;
      ScalarF y = std::get<MinY>(bounds) + yratio * (std::get<MaxY>(bounds) - std::get<MinY>(bounds));
      size_t yindex = zindex + yi*nx;

      for (size_t xi=0; xi<nx; ++xi) {
        ScalarF xratio = (ScalarF)xi / (ScalarF)nx;
        ScalarF x = std::get<MinX>(bounds) + xratio * (std::get<MaxX>(bounds) - std::get<MinX>(bounds));
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
    points = Points::New(VTK_DOUBLE);

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
    ScalarF z0 = std::get<MinZ>(bounds) + z0ratio * (std::get<MaxZ>(bounds) - std::get<MinZ>(bounds));
    ScalarF z1ratio = (ScalarF)(zi + 1) / (ScalarF)nz;
    ScalarF z1 = std::get<MinZ>(bounds) + z1ratio * (std::get<MaxZ>(bounds) - std::get<MinZ>(bounds));
    size_t zindex = zi*ny*nx;
    assert(("the later code expects z0 < z1, so sanity check here", z0 < z1));

    for (size_t yi=0; yi<ny; ++yi) {
      ScalarF y0ratio = (ScalarF)(yi + 0) / (ScalarF)ny;
      ScalarF y0 = std::get<MinY>(bounds) + y0ratio * (std::get<MaxY>(bounds) - std::get<MinY>(bounds));
      ScalarF y1ratio = (ScalarF)(yi + 1) / (ScalarF)ny;
      ScalarF y1 = std::get<MinY>(bounds) + y1ratio * (std::get<MaxY>(bounds) - std::get<MinY>(bounds));
      size_t yindex = zindex + yi*nx;
      assert(("the later code expects y0 < y1, so sanity check here", y0 < y1));

      for (size_t xi=0; xi<nx; ++xi, ++i) {
        ScalarF x0ratio = (ScalarF)(xi + 0) / (ScalarF)nx;
        ScalarF x0 = std::get<MinX>(bounds) + x0ratio * (std::get<MaxX>(bounds) - std::get<MinX>(bounds));
        ScalarF x1ratio = (ScalarF)(xi + 1) / (ScalarF)nx;
        ScalarF x1 = std::get<MinX>(bounds) + x1ratio * (std::get<MaxX>(bounds) - std::get<MinX>(bounds));
        size_t xindex = yindex + xi;
        assert(("the later code expects x0 < x1, so sanity check here", x0 < x1));

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

// helper function to write the rendered image as PPM file
static void writePPM(const char *fileName, int size_x, int size_y, const uint32_t *pixel) {
  using namespace std;

  FILE *file = fopen(fileName, "wb");
  if (!file) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size_x, size_y);
  unsigned char *out = (unsigned char *)alloca(3 * size_x);
  for (int y = 0; y < size_y; y++) {
    const unsigned char *in =
        (const unsigned char *)&pixel[(size_y - 1 - y) * size_x];
    for (int x = 0; x < size_x; x++) {
      out[3 * x + 0] = in[4 * x + 0];
      out[3 * x + 1] = in[4 * x + 1];
      out[3 * x + 2] = in[4 * x + 2];
    }
    fwrite(out, 3 * size_x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}


//---

int main(int argc, char **argv) {
  int provided;
  int success = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (success != MPI_SUCCESS) {
    fprintf(stderr, "Error while initializing MPI\n");
    return 1;
  }

  if (provided != MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "MPI provided the wrong level of thread support\n");
    return 1;
  }

  using Controller = vtkMPIController;
  vtkNew<Controller> controller;
  controller->Initialize(&argc, &argv, /* initializedExternally= */1);
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
  int opt_width;
  int opt_height;
  int opt_spp;

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
  opt_width = 256;
  opt_height = 256;
  opt_spp = 1;

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
  ARG("-width") opt_width = std::stoi(ARGVAL);
  ARG("-height") opt_height = std::stoi(ARGVAL);
  ARG("-spp") opt_spp = std::stoi(ARGVAL);

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

  if (controller->Barrier(), opt_rank == 0) {
    mandelbrots[0].debug(Mandelbrot::Debug::OnlyNsteps);
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
    
    // distributedDataFilter->UpdateDataObject();
    // {
    //   auto dataObject = distributedDataFilter->GetOutputDataObject(0);
    //   auto grid = vtkUnstructuredGrid::SafeDownCast(dataObject);
    //   auto points = vtkPoints::New();
    //   points->SetDataType(VTK_FLOAT);
    //   grid->SetPoints(points);
    // }

    DEBUG_RANK0(<< "D3: " << *distributedDataFilter);
    distributedDataFilter->Update();

    using KdTree = vtkPKdTree;
    vtkSmartPointer<KdTree> kdTree = distributedDataFilter->GetKdtree();

    DEBUG_RANK0(<< "kdTree: " << *kdTree);

    unstructuredGrid = UnstructuredGrid::SafeDownCast(distributedDataFilter->GetOutput());
  }

  DEBUG(<< "ugrid: " << *unstructuredGrid);

  // using CompositeDataIterator = vtkCompositeDataIterator;
  // vtkSmartPointer<CompositeDataIterator> compositeDataIterator = multiBlockDataSet->NewIterator();
  // compositeDataIterator->InitTraversal();
  // while (!compositeDataIterator->IsDoneWithTraversal()) {
  //   using DataObject = vtkDataObject;
  //   DataObject *dataObject = compositeDataIterator->GetCurrentDataObject();

  //   std::cout << *dataObject << std::endl;

  //   compositeDataIterator->GoToNextItem();
  // }

  if (controller->Barrier(), opt_rank == 0) {
    using ObjectFactory = vtkObjectFactory;
    using ObjectFactoryCollection = vtkObjectFactoryCollection;
    ObjectFactoryCollection *objectFactoryCollection = ObjectFactory::GetRegisteredFactories();

    using CollectionSimpleIterator = vtkCollectionSimpleIterator;
    CollectionSimpleIterator collectionSimpleIterator;
    objectFactoryCollection->InitTraversal(collectionSimpleIterator);
    ObjectFactory *objectFactory{nullptr};
    while ((objectFactory = objectFactoryCollection->GetNextObjectFactory(collectionSimpleIterator)) != nullptr) {
      std::cerr << *objectFactory << std::endl;
    }
  }

# if 0

  using RenderWindow = vtkRenderWindow;
  vtkNew<RenderWindow> renderWindow;
  renderWindow->EraseOn();
  renderWindow->ShowWindowOff();
  renderWindow->UseOffScreenBuffersOn();
  renderWindow->SetSize(opt_width, opt_height);
  renderWindow->SetMultiSamples(0);

  using RenderPass = vtkOSPRayPass;
  vtkNew<RenderPass> renderPass;
  renderPass->DebugOn();

  using PiecewiseFunction = vtkPiecewiseFunction;
  vtkNew<PiecewiseFunction> piecewiseFunction;
  piecewiseFunction->AddPoint(0.0, 0.5);
  piecewiseFunction->AddPoint(1.0, 1.0);

  using ColorTransferFunction = vtkColorTransferFunction;
  vtkNew<ColorTransferFunction> colorTransferFunction;
  colorTransferFunction->SetColorSpaceToRGB();
  colorTransferFunction->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(1.0, 0.0, 1.0, 0.0);

  using VolumeProperty = vtkVolumeProperty;
  vtkNew<VolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(piecewiseFunction);
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  using VolumeMapper = vtkUnstructuredGridVolumeRayCastMapper;
  vtkNew<VolumeMapper> volumeMapper;
  volumeMapper->SetInputData(unstructuredGrid);

  using Volume = vtkVolume;
  vtkNew<Volume> volume;
  volume->SetProperty(volumeProperty);
  volume->SetMapper(volumeMapper);

  using Renderer = vtkRenderer;
  vtkNew<Renderer> renderer;
  renderer->SetBackground(1.0, 1.0, 1.0);
  // renderer->SetPass(renderPass);
  renderer->AddVolume(volume);

  using Camera = vtkCamera;
  Camera *camera = renderer->GetActiveCamera();
  camera->SetPosition(0, 0, -4);

  renderWindow->AddRenderer(renderer);
  renderer->SetRenderWindow(renderWindow);

  using WindowToImageFilter = vtkWindowToImageFilter;
  vtkNew<WindowToImageFilter> windowToImageFilter;
  windowToImageFilter->SetInput(renderWindow);
  windowToImageFilter->Update();

  using JPEGWriter = vtkJPEGWriter;
  vtkNew<JPEGWriter> jpegWriter;
  jpegWriter->SetFileName("tmp/out.jpg");
  jpegWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
  jpegWriter->Write();

# elif 1

  // TODO(th): Try using the vtk rendering itself, without OSPRay

  OSPDevice device{nullptr};
  std::vector<uint8_t> volumeCellType{};
  OSPData volumeCellTypeData{nullptr};
  std::vector<uint32_t> volumeCellIndex{};
  OSPData volumeCellIndexData{nullptr};
  std::vector<float> volumeVertexPosition{};
  OSPData volumeVertexPositionData{nullptr};
  std::vector<float> volumeCellData{};
  OSPData volumeCellDataData{nullptr};
  std::vector<uint32_t> volumeIndex{};
  OSPData volumeIndexData{nullptr};
  OSPVolume volume{nullptr};
  std::vector<float> transferFunctionColor{};
  OSPData transferFunctionColorData{nullptr};
  std::vector<float> TransferFunctionOpacity{};
  OSPData transferFunctionOpacityData{nullptr};
  OSPTransferFunction transferFunction{nullptr};
  OSPVolumetricModel volumetricModel{nullptr};
  OSPGroup group{nullptr};
  OSPInstance instance{nullptr};
  OSPLight light{nullptr};
  std::vector<float> worldRegion;
  OSPData worldRegionData{nullptr};
  OSPWorld world{nullptr};
  OSPCamera camera{nullptr};
  OSPRenderer renderer{nullptr};
  OSPFrameBuffer frameBuffer{nullptr};
  OSPFuture future;

  // ospLoadModule("mpi");

  // device = ospNewDevice("mpiDistributed");
  // ospDeviceCommit(device);
  // ospSetCurrentDevice(device);

  ospInit(nullptr, nullptr);

  {
    using Array = vtkUnsignedCharArray;
    Array *array = unstructuredGrid->GetCellTypesArray();
    volumeCellType.resize(array->GetNumberOfValues(), 0);
    for (size_t i=0; i<array->GetNumberOfValues(); ++i) {
      volumeCellType[i] = array->GetValue(i);
    }
    volumeCellTypeData = ospNewSharedData(volumeCellType.data(), OSP_UCHAR,
                                          array->GetNumberOfTuples(), 0,
                                          array->GetNumberOfComponents(), 0,
                                          1, 0);
    ospCommit(volumeCellTypeData);
  }

  {
    using Array = vtkIdTypeArray;
    Array *array = unstructuredGrid->GetCellLocationsArray();
    volumeCellIndex.resize(array->GetNumberOfValues(), 0);
    for (size_t i=0; i<array->GetNumberOfValues(); ++i) {
      volumeCellIndex[i] = array->GetValue(i);
    }
    volumeCellIndexData =
      ospNewSharedData(volumeCellIndex.data(), OSP_UINT,
                       array->GetNumberOfTuples(), 0,
                       array->GetNumberOfComponents(), 0,
                       1, 0);
    ospCommit(volumeCellIndexData);
  }

  {
    using Array = vtkDoubleArray;
    Array *array = Array::SafeDownCast(unstructuredGrid->GetPoints()->GetData());
    volumeVertexPosition.resize(array->GetNumberOfValues(), 0.0f);
    for (size_t i=0; i<array->GetNumberOfValues(); ++i) {
      volumeVertexPosition[i] = array->GetValue(i);
    }
    volumeVertexPositionData =
      ospNewSharedData(volumeVertexPosition.data(), OSP_VEC3F,
                       array->GetNumberOfTuples(), 0,
                       array->GetNumberOfComponents() / 3, 0,
                       1, 0);
    ospCommit(volumeVertexPositionData);
  }

  {
    using Array = vtkUnsignedShortArray;
    Array *array = Array::SafeDownCast(unstructuredGrid->GetCellData()->GetScalars());
    volumeCellData.resize(array->GetNumberOfValues(), 0.0f);
    for (size_t i=0; i<array->GetNumberOfValues(); ++i) {
      volumeCellData[i] = array->GetValue(i);
    }
    volumeCellDataData =
      ospNewSharedData(volumeCellData.data(), OSP_FLOAT,
                       array->GetNumberOfTuples(), 0,
                       array->GetNumberOfComponents(), 0,
                       1, 0);
    ospCommit(volumeCellDataData);
  }

  {
    using Array = vtkTypeInt64Array;
    Array *array = Array::SafeDownCast(unstructuredGrid->GetCells()->GetConnectivityArray());
    volumeIndex.resize(array->GetNumberOfValues(), 0.0f);
    for (size_t i=0; i<array->GetNumberOfValues(); ++i) {
      volumeIndex[i] = array->GetValue(i);
    }
    volumeIndexData =
      ospNewSharedData(volumeIndex.data(), OSP_UINT,
                       array->GetNumberOfTuples(), 0,
                       array->GetNumberOfComponents(), 0,
                       1, 0);
    ospCommit(volumeIndexData);
  }

  OSPGeometry geometry;
  geometry = ospNewGeometry("sphere");
  // https://ospray.org/documentation.html#geometries
  // https://ospray.org/documentation.html#spheres
  ospSetObject(geometry, "sphere.position", volumeVertexPositionData);
  ospSetFloat(geometry, "radius", 0.01);
  ospCommit(geometry);

  OSPMaterial material;
  material = ospNewMaterial(nullptr, "obj");
  // https://ospray.org/documentation.html#materials
  // https://ospray.org/documentation.html#obj-material
  ospSetVec3f(material, "kd", 0.8, 0.8, 0.8);
  ospCommit(material);

  OSPGeometricModel geometricModel;
  geometricModel = ospNewGeometricModel();
  // https://ospray.org/documentation.html#geometries
  // https://ospray.org/documentation.html#geometricmodels
  ospSetObject(geometricModel, "geometry", geometry);
  ospSetObject(geometricModel, "material", material);
  ospCommit(geometricModel);

  volume = ospNewVolume("unstructured");
  // https://ospray.org/documentation.html#volumes
  // https://ospray.org/documentation.html#unstructured-volume
  ospSetObject(volume, "vertex.position", volumeVertexPositionData);
  // ospSetObject(volume, "vertex.data", nullptr);
  ospSetObject(volume, "index", volumeIndexData);
  ospSetBool(volume, "indexPrefixed", false);
  ospSetObject(volume, "cell.index", volumeCellIndexData);
  ospSetObject(volume, "cell.data", volumeCellDataData);
  ospSetObject(volume, "cell.type", volumeCellTypeData);
  // ospSetBool(volume, "hexIterative", false);
  // ospSetBool(volume, "precomputedNormals", false);
  ospSetFloat(volume, "background", 0.0f);
  ospCommit(volume);

  transferFunctionColor.clear();
  transferFunctionColor.insert(transferFunctionColor.end(), {
    (opt_rank % 3 == 0 ? 1.0f : 0.0f),
    (opt_rank % 3 == 1 ? 1.0f : 0.0f),
    (opt_rank % 3 == 2 ? 1.0f : 0.0f),
    (opt_rank % 3 == 0 ? 1.0f : 0.0f),
    (opt_rank % 3 == 1 ? 1.0f : 0.0f),
    (opt_rank % 3 == 2 ? 1.0f : 0.0f),
  });
  transferFunctionColorData = ospNewSharedData(transferFunctionColor.data(), OSP_VEC3F, transferFunctionColor.size() / 3);
  ospCommit(transferFunctionColorData);

  TransferFunctionOpacity.clear();
  TransferFunctionOpacity.insert(TransferFunctionOpacity.end(), {
    0.0f,
    1.0f,
  });
  transferFunctionOpacityData = ospNewSharedData(TransferFunctionOpacity.data(), OSP_FLOAT, TransferFunctionOpacity.size() / 1);
  ospCommit(transferFunctionOpacityData);

  transferFunction = ospNewTransferFunction("piecewiseLinear");
  ospSetObject(transferFunction, "color", transferFunctionColorData);
  ospSetObject(transferFunction, "opacity", transferFunctionOpacityData);
  ospSetVec2f(transferFunction, "valueRange", (float)0.0f, (float)opt_nsteps);
  ospCommit(transferFunction);

  volumetricModel = ospNewVolumetricModel(nullptr);
  ospSetObject(volumetricModel, "volume", volume);
  ospSetObject(volumetricModel, "transferFunction", transferFunction);
  ospCommit(volumetricModel);

  group = ospNewGroup();
  ospSetObjectAsData(group, "volume", OSP_VOLUMETRIC_MODEL, volumetricModel);
  // ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, geometricModel);
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
  ospSetFloat(camera, "aspect", (float)opt_width / (float)opt_height);
  ospSetVec3f(camera, "position", 0.0f, 0.0f, 10.0f);
  ospSetVec3f(camera, "direction", 0.0f, 0.0f, -1.0f);
  ospSetVec3f(camera, "up", 0.0f, 1.0f, 0.0f);
  ospCommit(camera);

  renderer = ospNewRenderer("scivis");
  ospSetInt(renderer, "pixelSamples", opt_spp);
  ospSetVec3f(renderer, "backgroundColor", 0.0f, 0.0f, 0.0f);
  ospCommit(renderer);

  frameBuffer = ospNewFrameBuffer(opt_width, opt_height, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
  ospCommit(frameBuffer);

  ospResetAccumulation(frameBuffer);
  future = ospRenderFrame(frameBuffer, renderer, camera, world);
  ospWait(future, OSP_TASK_FINISHED);
  ospRelease(future);
  future = nullptr;

  for (size_t i=0; i<opt_nprocs; ++i) {
    if (controller->Barrier(), i == opt_rank) {
      std::string filename = std::string("vtkOSPRay.") + std::to_string(opt_rank) + std::string(".ppm");
      const void *fb = ospMapFrameBuffer(frameBuffer, OSP_FB_COLOR);
      writePPM(filename.c_str(), opt_width, opt_height, static_cast<const uint32_t *>(fb));
      ospUnmapFrameBuffer(fb, frameBuffer);
    }
  }

  // MPI_Finalize();

# endif

  return 0;
}
