/**
 *
 */

#include <vtkNew.h>
#include <vtkOSOpenGLRenderWindow.h>
#include <vtkOSPRayPass.h>
#include <vtkOSPRayRendererNode.h>
#include <vtkRenderer.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    using RenderWindow = vtkOSOpenGLRenderWindow;

    vtkNew<RenderWindow> renderWindow;
    renderWindow->EraseOn();
    renderWindow->ShowWindowOff();
    renderWindow->UseOffScreenBuffersOn();
    renderWindow->SetSize(512, 512);
    renderWindow->SetMultiSamples(0);

    using RenderPass = vtkOSPRayPass;
    vtkNew<RenderPass> renderPass;

    using Renderer = vtkRenderer;
    vtkNew<Renderer> renderer;
    renderer->SetBackground(0.2, 0.2, 0.2);
    renderer->SetRenderWindow(renderWindow);
    renderer->SetPass(renderPass);

    renderWindow->Render();

    return 0;
}
