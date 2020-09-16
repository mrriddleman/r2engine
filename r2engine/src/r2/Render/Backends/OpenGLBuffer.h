////
////  openglbuffer.h
////  r2engine
////
////  created by serge lansiquot on 2019-12-26.
////
//
//#ifndef openglbuffer_h
//#define openglbuffer_h
//
//#include "r2/render/renderer/bufferlayout.h"
//#include "r2/render/renderer/vertex.h"
//#include "glad/glad.h"
//#include "r2/render/renderer/renderertypes.h"
//
//namespace r2::draw::opengl
//{
//    
//    
//    struct framebuffer
//    {
//        u32 fbo = empty_buffer;
//        u32 width = 0, height = 0;
//        std::vector<u32> colorattachments;
//        u32 depthattachment = empty_buffer;
//        u32 stencilattachment = empty_buffer;
//    };
//    
//    struct renderbuffer
//    {
//        u32 rbo = empty_buffer;
//        u32 width = 0, height = 0;
//    };
//    
//    void create(vertexarraybuffer& buf);
//    void create(vertexbuffer& buf, float * vertices, u64 size, u32 type);
//    void create(vertexbuffer& buf, const std::vector<vertex>& vertices, u32 type);
//    void create(vertexbuffer& buf, const vertex* vertices, u64 size, u32 type);
//    void create(vertexbuffer& buf, const bonedata* bonedata, u64 size, u32 type);
//    
//    void create(vertexbuffer& buf, void* data, u64 size, u32 type);
//    
//    void create(indexbuffer& buf, const u32* indices, u64 size, u32 type);
//    
//    void create(framebuffer& buf, u32 width, u32 height);
//    void create(framebuffer& buf, const r2::util::size& size);
//    void create(renderbuffer& buf, u32 width, u32 height);
//    
//    void destroy(vertexarraybuffer& buf);
//    void destroy(vertexbuffer& buf);
//    void destroy(indexbuffer& buf);
//    void destroy(framebuffer& buf);
//    void destroy(renderbuffer& buf);
//    
//    bufferlayouthandle createbufferlayoutindexed(const bufferlayout& bufferlayout, const vertexbuffer& vbuffer, const indexbuffer& ibuffer);
//    void setindexbuffer(vertexarraybuffer& arraybuf, const indexbuffer& buf);
//    
//    void bind(const vertexarraybuffer& buf);
//    void unbind(const vertexarraybuffer& buf);
//    
//    void bind(const vertexbuffer& buf);
//    void unbind(const vertexbuffer& buf);
//    
//    void bind(const indexbuffer& buf);
//    void unbind(const indexbuffer& buf);
//    
//    void bind(const framebuffer& buf);
//    void unbind(const framebuffer& buf);
//    
//    void bind(const renderbuffer& buf);
//    void unbind(const renderbuffer& buf);
//    
//    u32 attachtexturetoframebuffer(framebuffer& buf, bool alpha = false, glenum filter = gl_linear);
//    u32 attachhdrtexturetoframebuffer(framebuffer& buf, glenum internalformat, glenum filter = gl_linear, glenum wrapmode = gl_repeat);
//    
//    
//    
//    u32 attachdepthandstencilforframebuffer(framebuffer& buf);
//    void attachdepthandstencilforrenderbuffertoframebuffer(const framebuffer& framebuf, const renderbuffer& rbuf);
//    void attachdepthbufferforrenderbuffertoframebuffer(const framebuffer& framebuf, const renderbuffer& rbuf);
//    u32 attachdepthtoframebuffer(framebuffer& buf);
//    
//    void attachdepthcubemaptoframebuffer(framebuffer& buf, u32 depthcubemap);
//    
//    u32 attachmultisampletexturetoframebuffer(framebuffer& buf, u32 samples);
//    void attachdepthandstencilmultisampleforrenderbuffertoframebuffer(const framebuffer& framebuf, const renderbuffer& rbuf, u32 samples);
//    
//}
//
//#endif /* openglbuffer_h */
