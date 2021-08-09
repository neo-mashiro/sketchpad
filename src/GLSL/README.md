// transform矩阵很少在GLSL中完成，通常是用GLM在C++代码中算好矩阵, send it to GLSL, 在GLSL里只做multiplication乘法。
// GPU对矩阵乘法是优化过的，矩阵乘法都要放在shader里（主要是vertex shader，对每个vertex apply）。
// 经过shader后，GPU硬件中的厂商的OpenGL代码会自动计算"divide by W"的步骤

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


// add this mode when you are working with the Mesh chapter
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // draw wireframe
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


*/

once we save a shader program to disk as a binary file, it becomes permanent and can be loaded into OpenGL again.
It should be noted that only the compiled data gets saved into the binary, but not the program object id, so it's a piece of data not owned by any shader program. Later when we load it into OpenGL, we still need to create a shader program to which the binary data should be assigned. This essentially means that a pre-compiled shader binary can be loaded by multiple entities, it can be shared by any number of shader programs (each has a different id) with no conflicts.
















































---
