//用4倍抗锯齿的话，每个pixel里面会有4个fragment

//transform矩阵很少在GLSL中完成，通常是用GLM在C++代码中算好矩阵, send it to GLSL, 在GLSL里只做multiplication乘法。
//CPU擅长做通用计算。GPU是并行的，GPU对矩阵乘法是优化过的，矩阵乘法都要放在shader里（主要是vertex shader，对每个vertex apply）。

//GLM如何定义各种矩阵，以及做各种tranformation
https://learnopengl.com/Getting-started/Transformations

// 三大转换矩阵 1. Model matrix: Model Space (local) -> World Space (global)
Model_matrix = TranslationMatrix * RotationMatrix * ScaleMatrix;
TransformedVector = Model_matrix * OriginalVector;  // Model Space (local) -> World Space (global)

// 2. View matrix: World Space -> Camera Space
glm::mat4 CameraMatrix = glm::lookAt(
    cameraPosition, // the position of your camera, in world space
    cameraTarget,   // where you want to look at, in world space
    upVector        // probably glm::vec3(0,1,0)
);

// 3. Projection matrix: Camera Space -> Homogeneous Space (the viewing cube, range (-1, 1) in all xyz axes).
// 乘以这个矩阵后，实质上是把所有可见的物体变成相机frustum的形状，而把相机frustum变成我们的viewing cube立方体。
glm::mat4 projectionMatrix = glm::perspective(
    glm::radians(FoV), // The vertical Field of View, in radians
    4.0f / 3.0f,       // Aspect Ratio. Depends on the size of your window. usually 16:9
    0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
    100.0f             // Far clipping plane. Keep as little as possible.
);

// 三大矩阵组合起来成为一个大矩阵：the ModelViewProjection matrix，简称MVP矩阵。
glm::mat4 MVP = P * V * M;                  // compute the matrix in C++
transformed_vertex = MVP * in_vertex;       // apply it in GLSL

// step 1. compute the MVP matrix in C++, send it to GLSL
GLuint MVP_uid = glGetUniformLocation(PO, "MVP");  // query location (ONLY ONCE in Init())

glm::mat4 P = glm::perspective(glm::radians(45.0f), (float) width / (float)height, 0.1f, 100.0f);
glm::mat4 P = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates
glm::mat4 V = glm::lookAt(
    glm::vec3(4,3,3), // Camera is at (4,3,3), in World Space
    glm::vec3(0,0,0), // and looks at the origin
    glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
);
glm::mat4 M = glm::mat4(1.0f);  // Model matrix : an identity matrix (model will be at the origin)
glm::mat4 MVP = P * V * M; // 乘的顺序是反过来的

glUniformMatrix4fv(MVP_uid, 1, GL_FALSE, &MVP[0][0]);  // set the MVP uniform (EVERY FRAME! each model has a different MVP)

// step 2. apply MVP in GLSL
layout(location = 0) in vec3 position;
uniform mat4 MVP;
void main() {
    gl_Position =  MVP * vec4(position, 1.0f);
}

// 经过shader后，GPU硬件中的厂商的OpenGL代码会自动计算"divide by W"的步骤

// VAO是一个用于管理VBO和IBO的对象，VAO中存储着：哪个VBO绑定了什么数据，对应的IBO是什么，等等之类的信息。
// 一个程序至少要有一个VAO，一个VAO可以管理多个VBOIBO。也可以有多个VAO，用于管理不同的要draw的物体。
// 如果只有一个VAO，但是要画多个物体，那么每次draw call之前，都要手动的绑定VBO，设置vertex的attributes，因为每个物体的VBO数据不一样，或是时刻在变化的。当然了，也可以每个物体都有一个单独的VAO，这个VAO包含了draw该物体需要知道的所有VBO,IBO的信息，都是绑定好了的，那么每次draw一个物体前，就只需要把OpenGL状态机的状态bind到这个VAO上就好了，这也是OpenGL推荐的做法。不过，performance上哪种更快没有定论，要根据自己的生产环境去benchmark，profile测试一下性能。不考虑性能的情况下，用多个VAO来管理程序会显得更方便。
glBindVertexArray(VAO);  // 下面的代码的所有bind都会被关联记录到这个VAO中，until这个VAO被解绑为止
//如果只有一个VAO，又有多个draw的物体或者物体VBO是动态的，那么每次draw call前都要加上下面这三行，放在display callback里。
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glEnableVertexAttribArray(0);  // 这里0其实是VAO array中的一个指针
glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);  //这里是告诉VBO我的数据的layout是怎样的，但实际上信息是绑到VAO里的

// some legacy OpenGL (deprecated, don't use)
glMatrixMode(GL_PROJECTION);  // activate the built-in projection matrix to modify it
glLoadIdentity();  // clean out any old leftover matrices
gluPerspective(90.0, 800/600, 0.01, 100.0);
glTranslatef(0,0,-2.0);
glRotatef();
glScalef();
glMultMatrixf(&viewMatrix[0][0]);
glMatrixMode(GL_MODELVIEW);  // activate the built-in model-view matrix to modify it
gluLookAt(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
glPushMatrix();  // save coordinate system
glPopMatrix();   // restore/load coordinate system

// some legacy GLSL (deprecated, don't use)
gl_ModelViewMatrix = mat4(...);
gl_ModelViewMatrixProjection = mat4(...);
attribute vec3 position;
varying vec2 myTexCoord;



// Try to not overburden the fragment processors of your GPU.
// it is recommended to do computations in the vertex shader rather than in the fragment shader, because
// for every vertex shader invocation, the fragment shader could be invoked hundreds or thousands of times more.
// (this is because the number of fragments is far more >> than the number of vertices, think of fragments as pixels)

// 尽可能的多使用Swizzle操作，Swizzle masks are essentially free in hardware
// Swizzle有三种表示方式，随意使用，xyzw, rgba, stpq
in vec4 in_pos;
// The following two lines:
gl_Position.x = in_pos.x;
gl_Position.y = in_pos.y;
// can be simplified to:
gl_Position.xy = in_pos.xy;  // much more efficient and faster

// 除法是比较昂贵的，通常需要cost额外的计算cycle，可能的情况下，尽量改成做乘法
vec4 x = (value / 2.0f);
vec4 x = (value * 0.5f);  // much faster

// 同样的操作，尽可能使用built-in的函数，不要自己计算，built-in的函数是优化过的，要快很多
x = a * t + b * (1 - t);  // slow
x = mix(a, b, t);         // fast

vec3 a;
value = a.x + a.y + a.z;        // slow
value = dot(a, vec4(1.0).xyz);  // fast

// 现实的应用中，关于VBO的使用是很有讲究的，当涉及到许多个mesh的时候，是分开bind多个VBO好，还是组合在一个大的VBO里好，是很复杂的。
// 这个问题要考虑许多因素，比如用大的VBO可以减少切换上下文的开销，但是每次draw一个mesh的时候，其他mesh的资源是被浪费的，等等，有很多权衡取舍。
// 此外，还要考虑是否是用了batch rendering，考虑mesh的大小，以及这些mesh是否需要频繁的更新，有时候可能还要用glBufferSubData来对mesh分层。
// 目前暂时不要考虑这些优化的问题，只做一个可以学习技术的项目就好了。

// GLSL中创建矩阵的时候，用vec来构建，但是要注意，矩阵是用column major order来创建的，每个vec必须是列，而不是行
mat2(vec2, vec2);
mat3(vec3, vec3, vec3);
mat4(vec4, vec4, vec4, vec4);

// GLSL中的float，不需要以f结尾，这点和C++ C#不一样。不要写f，这样看上去整洁一点。
float x = 3.5;
float y = 4.2f;  // unnecessary

/*
频幕上最终能看到的vertex全都是在Normalized Device Coordinates(NDC)空间里的，俗称NDC space。它就是xyz三个轴都在[-1,1]之间的一个立方体范围。
vertex shader的最终输出的gl_Position，就是把所有vertex的position转换到NDC空间。
NDC以外的vertex会被clip丢弃掉。
被转换到NDC空间的点，然后会通过glViewport转换为屏幕上的screen-space的点，这些点是最终我们看到的，也是fragment shader的input。

绑着VAO的时候，接下来开始设置VBO和IBO的各种layout，VBO设置好了以后，是可以安全的unbind的，但其实不需要，而IBO设置好了以后，千万不要在解绑VAO之前就unbind，否则刚刚的IBO就白设置了，VAO会抹掉之前的设置信息。
总之统一一下，绑着VAO的时候，再开始绑VBO并设置各种attrib，再绑IBO，记住，VBO和IBO都不要做任何手动unbind的动作，设置完成后直接unbind VAO就好了，这样的话，VAO当中会完整的保存着VBO+IBO的设置信息，并且在VAO解绑的同时它会自动解绑VBO+IBO。最后，每次要draw的时候，就只要bind到对应的VAO就好了。
如果你只draw一个物体，或只有一个VAO的话，每次在display callback里画完了之后也不用glBindVertexArray(0)，多此一举。

对于GLSL shader，const变量是不会被不同的shader stages之间shared的，也就是说，你在vertex shader里定义一个const变量，不会被fragment shader看到，哪怕你在fragment shader定义了一个一模一样的同名变量，那也是另一个变量。
而uniform是会被不同的shader stage之间共享的，假如你在vs和fs中都定义了一个同名的uniform变量，当vs和fs被link起来编译成一个program object的时候，这个program里只会有一个该uniform，换句话说，如果vs中定义了一个uniform A，fs中又定义了一个uniform也叫A，类型声明完全相同，这两个A实际上是同一个uniform变量，所以你只需要在opengl代码中从PO里面query一次这个uniform的location，不用query两次，也只需要给它赋值一次，不用赋两次，你赋值的时候，vs和fs都会同时接收到这个值。当然，现实中你一般不会这么做，既然uniform是global的，那么你只需要在用到这个uniform的shader里定义一次就好了，而不用在vs和fs里各定义一次，再从vs传到fs什么的。
但是，千万要注意，当你的程序里有多个PO，也就是有多个不同的shader program，这些program之间是完全独立的，他们里面的任何同名变量都不会被share，所以你在query location和赋值的时候，要对每个PO都分别操作一次。
尤其要注意的是，假如你定义了一个没有被shader程序用到的uniform，那么GLSL编译器会自动remove这个变量，于是你用glGetUniformLocation去query它的时候，结果会返回-1（但并不是报错），然后程序正常运行却显示黑屏，很难debug。

关于texture，FS里的sampler2D，samplerCube...等uniform，他们的location专门有个名称，叫做texture unit。
一般来说default都是texture unit 0，并且是默认开启的，我们并不需要显式地query这个uniform的location再用glUniform1i给它赋值我们的texture，当调用glBindTexture的时候，这是自动完成的。但有的显卡驱动可能没有default，所以unit 0通道不存在，这时候就必须要手动glUniform1i去赋值，否则黑屏。所以稳妥起见，每次都自己手动赋值比较清晰。

OpenGL至少会有16个通道，16个texture units。我们可以叠加或使用多个texture，先activate一个通道，然后去bind texture，就会自动bind到那个通道。
glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
glBindTexture(GL_TEXTURE_2D, texture);

normal map和bump map不是一回事。bump map是通过grayscale修改每个像素是偏黑还是偏白，提供像素的depth的错觉，但只有上下两个方向，它产生的detail是假的，通过旋转camera到不同的角度，就很容易发现，所以bump map只适合模拟大概的细节，优点是比较容易制作。
而normal map其实是新一代更先进的bump map，虽然normal map产生的depth细节也是假的，但它用的是RGB信息来对应3D空间的XYZ，给每个点都提供了normal的数据，参与shading的计算，所以哪怕camera换了角度也不会失真。

change to GLFW3!!!!
change to GLAD!!!!!
import IMGUI!!!!!


















// add this mode when you are working with the Mesh chapter
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // draw wireframe
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
改每个project的名字，和window title保持一致，否则不知道哪个是哪个，别管美丑，名字长无所谓
// 在mouse callback里，最后的一个项目把menu加进去，放些界面控制按钮，加上fps计数，成品



最后把所有的小项目整合成为一个deliverable，功能就和Unity的material选择器一样。
（做个大概雏形就可以，功能不要太复杂，因为这已经是game engine的范畴了，比Mana-Oasis优先度低很多）
默认的model是一个3D的单位sphere，即材质球，在viewport中显示。window比viewport大一点，周边加上menu和button和滑动bar，让用户可以自己选择各种选项，包括但不限于：texture（各种diffuse map，albedo，normal map之类的），light，skybox，或是上传自己的3D模型mesh数据来替代sphere，加上animation数据。然后要显示fps，要有一个plane，鼠标键盘要和FPS controller视角一样。











*/



















































// 争取把opengl官方tutorial都过一遍，然后加上GLDebugMessageCallback
// 然后把CSC461的东西加进来，还有antongerdelan的教程（比较简单）
// 中途一边把cherno的opengl视频概念全都看一遍

// paroj的那个教程是讲底层的，实际上不需要自己手动做那么多事，读一下看看概念就可以了

// alili的作业然后再做
// 所有opengl的东西，这个repo，争取赶在春假结束前全部搞定，然后做mana oasis

// 后面别折腾opengl，有时间都去留给GLSL，写shader









































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































//end
