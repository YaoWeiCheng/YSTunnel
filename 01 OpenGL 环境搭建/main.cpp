#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLFrame.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager		shaderManager;			//着色器管理器
GLMatrixStack		modelViewMatrix;		//模型视图矩阵
GLMatrixStack		projectionMatrix;		//投影矩阵
GLFrustum			viewFrustum;			//视景体
GLGeometryTransform	transformPipeline;		//几何变换管线

//4个批次容器类
GLBatch             floorBatch;//地面
GLBatch             ceilingBatch;//天花板
GLBatch             leftWallBatch;//左墙面
GLBatch             rightWallBatch;//右墙面

//深度初始值，-65。
GLfloat             viewZ = -65.0f;

// 纹理标识符号
#define TEXTURE_BRICK   0 //墙面
#define TEXTURE_FLOOR   1 //地板
#define TEXTURE_CEILING 2 //纹理天花板
#define TEXTURE_COUNT   3 //纹理个数 什么3个纹理数？因为左边和右边是一样的
GLuint  textures[TEXTURE_COUNT];//纹理标记数组

//文件tag名字数组
const char *szTextureFiles[TEXTURE_COUNT] = { "brick.tga", "floor.tga", "ceiling.tga" };



//菜单栏选择
void ProcessMenu(int value)
{
    GLint iLoop;
    
    for (iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        /**绑定纹理
         参数1：GL_TEXTURE_2D
         参数2：需要绑定的纹理对象
         */
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        /**配置纹理参数 glTextParameteri
         参数1：纹理模式
         参数2：纹理参数
         参数3：特定纹理参数
         */
        switch (value) {
            case 0:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST（最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                break;
            case 1:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR（线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                break;
            case 2:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_NEAREST（选择最邻近的Mip层，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
            case 3:
                 //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                break;
            case 4:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_NEAREST_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行最邻近过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
            case 5:
                //GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER(缩小过滤器)，GL_LINEAR_MIPMAP_LINEAR（在Mip层之间执行线性插补，并执行线性过滤，又称为三线性过滤）
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                break;
            case 6: //这里简单看一下就好了
                
               //设置各向异性过滤
                GLfloat fLargest;
                //获取各向异性过滤的最大数量
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
                //设置纹理参数
                glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
                break;
            case 7:
                //设置各向同性过滤，数量为1.0表示各向同性过
                glTexParameterf(GL_TEXTURE_2D, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
                break;
        }
    }

    //重绘制
    glutPostRedisplay();
}


//在这个函数里能够在渲染环境中进行任何需要的初始化，它这里的设置并初始化纹理对象
void SetupRC()
{
    GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFormat;
    GLint iLoop;
    
    //黑色背景
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    //初始化着色器管理器
    shaderManager.InitializeStockShaders();
    
    //加载纹理
    /** 分配纹理对象 glGenTextures
     参数1：纹理对象的数量
     参数2：纹理对象标识数组
     */
    glGenTextures(TEXTURE_COUNT, textures);
    
    //for循环绑定、设置、载入纹理和生成Mip图层
    for (iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        /**绑定纹理对象 glBindTexture
         参数1：纹理模式，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_3D
         参数2：需要绑定的纹理对象
         */
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        /**加载tga文件
         参数1：纹理文件名称
         参数2：文件宽度变量地址
         参数3：文件高度变量地址
         参数4：文件组件变量地址
         参数5：文件格式变量地址
         返回值：pBytes，指向图像数据的指针
         */
        pBytes = gltReadTGABits(szTextureFiles[iLoop], &iWidth, &iHeight, &iComponents, &eFormat);
        
        
        //设置过滤器
        //GL_TEXTURE_MAG_FILTER（放大过滤器,GL_NEAREST(最邻近过滤)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //GL_TEXTURE_MIN_FILTER(缩小过滤器),GL_NEAREST(最邻近过滤)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //包装模式(环绕方式)
        //GL_TEXTURE_WRAP_S(s轴环绕),GL_CLAMP_TO_EDGE(环绕模式强制对范围之外的纹理坐标沿着合法的纹理单元的最后一行或一列进行采样)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //GL_TEXTURE_WRAP_T(T轴环绕)，GL_CLAMP_TO_EDGE(环绕模式强制对范围之外的纹理坐标沿着合法的纹理单元的最后一行或一列进行采样)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //加载纹理
        glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
        
        /**生成Mip图层 glGenerateMipmap
         参数1：纹理维度，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_2D
         */
        glGenerateMipmap(GL_TEXTURE_2D);
        
        //释放原始纹理数据，不在需要纹理原始数据了
        free(pBytes);
    }
    
    //建立几何图形
    GLfloat z;
    
    /**
     GLTools库中的容器类，GBatch，
     void GLBatch::Begin(GLenum primitive,GLuint nVerts,GLuint nTextureUnits = 0);
     参数1：图元枚举值,三角形扇形
     参数2：顶点数
     参数3：1组或者2组纹理坐标
     */
    floorBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    
    //参考笔记坐标图的地板图形分析
    //Z表示深度，隧道的深度
    for (z = 60.0f; z >= 0.0f; z -= 10.0f) {
        // 纹理坐标，可以参照之前的OpenGL学习之路(6.0) 基础纹理的纹理坐标
        //注意：需要注意的是，它们绘制的顺序不能交叉改变
        floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        //vFrontRight
        floorBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
    }
    
    floorBatch.End();
    
    //参照之前的OpenGL学习之路(6.0) 基础纹理的纹理坐标
    ceilingBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z);
    }
    ceilingBatch.End();
    
    //同上
    leftWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
    }
    leftWallBatch.End();
    
    //同上
    rightWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
    }
    rightWallBatch.End();
}

//关闭渲染环境
void ShutdownRC(void)
{
   //删除纹理，有三个
    glDeleteTextures(TEXTURE_COUNT, textures);
}


//前后移动视口来对方向键作出响应
void SpecialKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) {
        //平移的是深度值，z值
        viewZ += 0.5f;
    }
    
    if (key == GLUT_KEY_DOWN) {
        viewZ -= 0.5f;
    }
    
    //更新窗口，即可回调到RenderScene函数里
    glutPostRedisplay();
}

//改变视景体和视口，在改变窗口大小或初始化窗口调用
void ChangeSize(int w, int h)
{
    
    //将视口设置大小
    glViewport(0, 0, w, h);
    
    //生成透视投影
    viewFrustum.SetPerspective(80.0f, float(w)/float(h), 1.0f, 120.f);
    //获取投影矩阵并加载矩阵
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

//调用，绘制场景
void RenderScene(void)
{
    // 清除颜色缓存区
    glClear(GL_COLOR_BUFFER_BIT);
    
    //模型视图压栈
    modelViewMatrix.PushMatrix(); //括号为空的时候，表示压栈当前线程
    //Z轴平移ViewZ 距离
    modelViewMatrix.Translate(0.0f, 0.0f, viewZ);
    
    //纹理替换着色器管理器
    /*
     参数1：GLT_SHADER_TEXTURE_REPLACE（着色器标签）
     参数2：模型视图投影矩阵
     参数3：纹理层
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_REPLACE, transformPipeline.GetModelViewProjectionMatrix(),0);
    
    //绑定纹理
    /*
     参数1：纹理模式，GL_TEXTURE_1D、GL_TEXTURE_2D、GL_TEXTURE_3D
     参数2：需要绑定的纹理
     */
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_FLOOR]);
    floorBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CEILING]);
    ceilingBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_BRICK]);
    leftWallBatch.Draw();
    rightWallBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    
    glutSwapBuffers();
    

}



int main(int argc, char *argv[])
{
    gltSetWorkingDirectory(argv[0]);
    
    // 标准初始化
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Tunnel");
    glutReshapeFunc(ChangeSize);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    
    // 添加菜单入口，改变过滤器
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("GL_NEAREST",0);
    glutAddMenuEntry("GL_LINEAR",1);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST",2);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 3);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 4);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 5);
    glutAddMenuEntry("Anisotropic Filter", 6);
    glutAddMenuEntry("Anisotropic Off", 7);
    
    
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    
    // 启动循环，关闭纹理
    SetupRC();
    glutMainLoop();
    ShutdownRC();
    
    return 0;
}



