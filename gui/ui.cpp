//
// open horizon -- undefined_darkness@outlook.com
//

#include "ui.h"
#include "containers/fhm.h"
#include "renderer/shared.h"

namespace gui
{
//------------------------------------------------------------

const int elements_per_batch = 200;

//------------------------------------------------------------

void render::init()
{
    if (m_tex.is_valid()) //already initialised
        return;

    m_tex.create();
    m_color.create();
    m_tr.create();
    m_tc_tr.create();

    struct vert { float x,y,s,t,i; } verts[elements_per_batch][4];

    uint16_t inds[elements_per_batch][6];

    for (int i = 0; i < elements_per_batch; ++i)
    {
        auto &v = verts[i];
        for(int j=0;j<4;++j)
        {
            v[j].x=j>1?-1.0f:1.0f,v[j].y=j%2?1.0f:-1.0f;
            v[j].s=j>1? 0.0f:1.0f,v[j].t=j%2?1.0f:0.0f;
            if (nya_render::get_render_api() == nya_render::render_api_directx11)
                v[j].t=1.0f-v[j].t;
            v[j].i = i;
        }

        inds[i][0] = i * 4;
        inds[i][1] = i * 4 + 1;
        inds[i][2] = i * 4 + 2;

        inds[i][3] = i * 4 + 2;
        inds[i][4] = i * 4 + 1;
        inds[i][5] = i * 4 + 3;
    }

    m_quads_mesh.set_vertex_data(verts,sizeof(verts[0][0]), elements_per_batch * 4);
    m_quads_mesh.set_vertices(0,2);
    m_quads_mesh.set_tc(0,2*4,3);
    m_quads_mesh.set_index_data(inds, nya_render::vbo::index2b, elements_per_batch * 6);

    vert line_verts[elements_per_batch];
    for (int i = 0; i < elements_per_batch; ++i)
    {
        auto &v = line_verts[i];
        v.x = v.y = 0.0f;
        v.i = i;
    }

    m_lines_mesh.set_vertex_data(line_verts, sizeof(line_verts[0]), elements_per_batch);
    m_lines_mesh.set_vertices(0, 2);
    m_lines_mesh.set_tc(0, 2*4, 3);
    m_lines_mesh.set_element_type(nya_render::vbo::line_strip);

    auto &pass = m_material.get_pass(m_material.add_pass(nya_scene::material::default_pass));
    pass.set_shader(nya_scene::shader("shaders/ui.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    pass.get_state().zwrite = false;
    pass.get_state().depth_test = false;
    m_material.set_texture("diffuse", m_tex);
    m_material.set_param(m_material.get_param_idx("color"), m_color);
    m_material.set_param_array(m_material.get_param_idx("transform"), m_tr);
    m_material.set_param_array(m_material.get_param_idx("tc transform"), m_tc_tr);
}

//------------------------------------------------------------

void render::draw(const std::vector<rect_pair> &elements, const nya_scene::texture &tex, const nya_math::vec4 &color) const
{
    if (!m_tex.is_valid()) //not initialised
        return;

    if (!m_width || !m_height)
        return;

    if (!tex.get_width() || !tex.get_height())
        return;

    if (elements.empty())
        return;

    assert(elements.size()<elements_per_batch); //ToDo: draw multiple times

    m_color->set(color);
    m_tex.set(tex);

    const float aspect = float(m_width) / m_height / (float(get_width()) / get_height() );
    const float iwidth = 1.0f / get_width();
    const float iheight = 1.0f / get_height();
    const float itwidth = 1.0f / tex.get_width();
    const float itheight = 1.0f / tex.get_height();

    for (int i = 0; i < (int)elements.size(); ++i)
    {
        auto &e = elements[i];
        m_tr->set_count(i + 1);
        m_tc_tr->set_count(i + 1);

        nya_math::vec4 pos;
        pos.z = float(e.r.w) * iwidth;
        pos.w = float(e.r.h) * iheight;
        pos.x = pos.z - 1.0 + 2.0 * e.r.x * iwidth;
        pos.y = -pos.w + 1.0 - 2.0 * e.r.y * iheight;
        if (aspect > 1.0)
        {
            pos.x /= aspect;
            pos.z /= aspect;
        }
        else
        {
            pos.y *= aspect;
            pos.w *= aspect;
        }
        m_tr->set(i, pos);

        nya_math::vec4 tc;
        tc.z = float(e.tc.w) * itwidth;
        tc.w = -float(e.tc.h) * itheight;
        tc.x = float(e.tc.x) * itwidth;
        tc.y = float(e.tc.y) * itheight - tc.w;
        m_tc_tr->set(i, tc);
    }

    m_material.internal().set(nya_scene::material::default_pass);
    m_quads_mesh.bind();
    m_quads_mesh.draw((unsigned int)elements.size() * 6);
    m_quads_mesh.unbind();
    m_material.internal().unset();

    static nya_scene::texture empty;
    m_tex.set(empty);
}

//------------------------------------------------------------

void render::draw(const std::vector<nya_math::vec2> &elements, const nya_math::vec4 &color) const
{
    if (!m_width || !m_height)
        return;

    if (elements.empty())
        return;

    assert(elements.size()<elements_per_batch); //ToDo: draw multiple times

    m_tex.set(shared::get_white_texture());
    m_color.set(color);

    const float aspect = float(m_width) / m_height / (float(get_width()) / get_height() );
    const float iwidth = 1.0f / get_width();
    const float iheight = 1.0f / get_height();

    for (int i = 0; i < (int)elements.size(); ++i)
    {
        auto &e = elements[i];
        m_tr->set_count(i + 1);
        m_tc_tr->set_count(i + 1);

        nya_math::vec4 pos;
        pos.x = -1.0 + 2.0 * e.x * iwidth;
        pos.y = 1.0 - 2.0 * e.y * iheight;
        if (aspect > 1.0)
            pos.x /= aspect;
        else
            pos.y *= aspect;
        m_tr->set(i, pos);
    }

    m_material.internal().set(nya_scene::material::default_pass);
    m_lines_mesh.bind();
    glLineWidth(1.0);
    m_lines_mesh.draw((unsigned int)elements.size());
    m_lines_mesh.unbind();
    m_material.internal().unset();

    static nya_scene::texture empty;
    m_tex.set(empty);
}

//------------------------------------------------------------

bool fonts::load(const char *name)
{
    fhm_file m;
    if (!m.open(name))
        return false;

    bool need_load_tex = false;
    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);
        if (t == 'RXTN')
        {
            if (m.get_chunk_size(i) <= 96)
                continue;

            if (!need_load_tex)
                continue;

            need_load_tex = false;
            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            m_font_texture = shared::get_texture(shared::load_texture(b.get_data(), b.get_size()));
            continue;
        }

        if (t == '\0FCA')
        {
            need_load_tex = true;
            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            nya_memory::memory_reader reader(b.get_data(),b.get_size());

            struct acf_header
            {
                char sign[4];
                uint32_t uknown;
                uint32_t uknown2;
                uint16_t unknown3_1;
                uint8_t unknown4_1;
                uint8_t fonts_count;
            };

            auto header = reader.read<acf_header>();

            std::vector<uint32_t> offsets(header.fonts_count);
            for (auto &o:offsets)
                o = reader.read<uint32_t>();

            m_fonts.resize(header.fonts_count);
            for (uint8_t i = 0; i < header.fonts_count; ++i)
            {
                reader.seek(offsets[i]);
                nya_memory::memory_reader r(reader.get_data(),reader.get_remained());
                auto &f = m_fonts[i];
                assert(r.get_data());
                f.name.assign((char *)r.get_data());
                r.seek(32);

                //print_data(r, r.get_offset(), sizeof(acf_font_header));

                auto header = r.read<acf_font_header>();
                r.skip(1024);

                f.t = header;

                for (uint32_t j = 0; j < header.char_count; ++j)
                {
                    //print_data(r, r.get_offset(), sizeof(acf_char));

                    auto c = r.read<acf_char>();

                    wchar_t char_code = ((c.char_code & 0xff) << 8) | ((c.char_code & 0xff00) >> 8);
                    auto &fc = f.chars[char_code];
                    fc.x = c.x, fc.y = c.y;
                    fc.w = c.width, fc.h = c.height;

                    fc.t = c;
                }
            }

            continue;
        }
 
        continue;

        char *c = (char *)&t;
        printf("%c%c%c%c\n",c[3],c[2],c[1],c[0]);

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader r(b.get_data(),b.get_size());
        print_data(r);
    }

    return true;
}

//------------------------------------------------------------

int fonts::draw_text(const render &r, const wchar_t *text, const char *font_name, int x, int y, const nya_math::vec4 &color) const
{
    if(!text || !font_name)
        return 0;

    const auto f = std::find_if(m_fonts.begin(), m_fonts.end(), [font_name](const font &fnt) { return fnt.name == font_name; });
    if (f == m_fonts.end())
        return 0;

    int width = 0;
    std::vector<rect_pair> elements;
    for (const wchar_t *c = text; *c; ++c)
    {
        rect_pair e;
        auto fc = f->chars.find(*c);
        if (fc == f->chars.end())
            continue;

        e.r.w = fc->second.w;
        e.r.h = fc->second.h;
        e.r.x = x + width;
        e.r.y = y;

        //ToDo

        //e.r.y -= fc->second.h;
        //e.r.y -= fc->second.t.unknown3[1] * 4;

        //e.r.y -= fc->second.h/2;
        //e.r.y -= fc->second.t.unknown3[1] * 2;

        //e.r.y -= fc->second.h * fc->second.t.unknown3[0] / 128;
        //e.r.y -= fc->second.t.unknown5 * fc->second.t.unknown3[0] / 128;

        //+ fc->second.t.unknown5;

        e.tc.w = fc->second.w;
        e.tc.h = fc->second.h;
        e.tc.x = fc->second.x;
        e.tc.y = fc->second.y;

        width += fc->second.w + fc->second.t.unknown3[3];

        elements.push_back(e);
    }

    r.draw(elements, m_font_texture, color);
    return width;
}

//------------------------------------------------------------

bool tiles::load(const char *name)
{
    fhm_file m;
    if (!m.open(name))
        return false;

    bool skip_fonts_texture = false;
    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);

        if (t == '\0FCA')
        {
            skip_fonts_texture = true;
            continue;
        }

        if (t == 'RXTN')
        {
            if (m.get_chunk_size(i) <= 96)
                continue;

            if (skip_fonts_texture)
            {
                skip_fonts_texture = false;
                continue;
            }

            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            m_textures.resize(m_textures.size() + 1);
            m_textures.back() = shared::get_texture(shared::load_texture(b.get_data(), b.get_size()));
            continue;
        }

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader reader(b.get_data(),b.get_size());

        if (t == ' MSH')
        {
            //ToDo
            continue;
        }

        if (t == ' DUH')
        {
            reader.skip(4);
            auto count = reader.read<uint32_t>();

            struct hud_chunk_info
            {
                uint32_t id;
                uint32_t offset;
                uint32_t size;
            };

            std::vector<hud_chunk_info> infos(count);
            for (auto &inf: infos)
                inf = reader.read<hud_chunk_info>();

            m_hud.resize(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                auto &inf = infos[i];
                reader.seek(inf.offset);
                nya_memory::memory_reader r(reader.get_data(), inf.size);

                struct hud_sub_chunk_info
                {
                    uint32_t unknown_6;
                    uint32_t unknown_1;
                    uint32_t unknown_17988; //DF\0\0
                    uint32_t offset;
                    uint32_t size;
                };

                auto info = reader.read<hud_sub_chunk_info>();
                assume(info.unknown_6 == 6);
                assume(info.unknown_1 == 1);
                assume(info.unknown_17988 == 17988);

                r.seek(info.offset);
                r = nya_memory::memory_reader(r.get_data(), info.size);
                auto count = r.read<uint32_t>();
                struct hud_sub_sub_info { uint32_t offset, size; };
                std::vector<hud_sub_sub_info> infos(count);
                for (auto &inf: infos)
                    inf = r.read<hud_sub_sub_info>();

                auto &h = m_hud[i];
                h.id = inf.id;

                m_hud_map[h.id] = i;

                for (auto &inf: infos)
                {
                    r.seek(inf.offset);

                    struct hud_type_header
                    {
                        uint32_t type;
                        uint32_t unknown; //often 0xffffffff
                        uint32_t unknown_0_or_1_or_2;
                        uint32_t unknown_0;
                    };

                    auto header = r.read<hud_type_header>();
                    assume(header.unknown_0_or_1_or_2 == 0 || header.unknown_0_or_1_or_2 == 1 || header.unknown_0_or_1_or_2 == 2);
                    assume(header.unknown_0 == 0);

                    if (header.type == 1)
                    {
                        hud_type1 ht1;
                        while(r.get_remained())
                        {
                            ht1.line_loops.resize(ht1.line_loops.size() + 1);
                            auto &loop = ht1.line_loops.back();
                            auto count = r.read<uint32_t>();
                            if (count == 1)
                            {
                                struct unknown_struct
                                {
                                    uint8_t unknown[4];
                                    uint32_t unknown_2;
                                    uint32_t unknown_0;
                                };

                                auto u = r.read<unknown_struct>();
                                assume(u.unknown_2 == 2);
                                assume(u.unknown_0 == 0);
                            }
                            else
                            {
                                assume(count != 0);
                                loop.resize(count);
                                for (auto &l: loop)
                                    l = r.read<nya_math::vec2>();
                            }
                        }

                        h.type1.push_back(ht1);
                    }
                    else if (header.type == 3)
                    {
                        h.type3.push_back(r.read<hud_type3>());
                    }
                    else if (header.type == 4)
                    {
                        h.type4.push_back(r.read<hud_type4>());
                        assume(h.type4.back().unknown_zero[0] == 0);
                        assume(h.type4.back().unknown_zero[1] == 0);
                        assume(h.type4.back().unknown_zero[2] == 0);
                    }
                    else if (header.type == 5)
                    {
                        //ToDo
                    }
                    else
                        assume(0 && header.type);
                }

                //print_data(r);
            }

            continue;
        }

        if (t == 'XTIU')
        {
            m_uitxs.resize(m_uitxs.size() + 1);
            auto &tx = m_uitxs.back();
            tx.header = reader.read<uitx_header>();
            assume(tx.header.zero[0] == 0 && tx.header.zero[1] == 0);
            tx.entries.resize(tx.header.count);
            for (auto &e: tx.entries)
                e = reader.read<uitx_entry>();
            continue;
        }

        if (t == '\0TCA')
        {
            //ToDo
            continue;
        }

        if (m.get_chunk_size(i) == 0)
            continue;

        char *c = (char *)&t;
        printf("%c%c%c%c %d %d %d %d size %ld\n",c[3],c[2],c[1],c[0],c[3],c[2],c[1],c[0],m.get_chunk_size(i));
        //continue;

        print_data(reader);
    }

    return true;
}

//------------------------------------------------------------

int tiles::get_id(int idx)
{
    if (idx < 0 || idx >= get_count())
        return -1;

    return m_hud[idx].id;
}

//------------------------------------------------------------

void tiles::draw(const render &r, int idx, int x, int y, const nya_math::vec4 &color)
{
    auto it = m_hud_map.find(idx);
    if (it == m_hud_map.end())
        return;

    auto &h = m_hud[it->second];
    for (auto &t1: h.type1)
    {
        for (auto &l: t1.line_loops)
        {
            auto loop = l;
            for (auto &l: loop)
            {
                l.x += x;
                l.y += y;
            }
            r.draw(loop, color);
        }
    }

    std::vector<rect_pair> rects;
    int tex_idx = -1;
    for (auto &t3: h.type3)
    {
        auto &e = m_uitxs[0].entries[t3.tile_idx];
        rect_pair rp;

        rp.tc.x = e.x;
        rp.tc.y = e.y;
        rp.tc.w = e.w;
        rp.tc.h = e.h;
        rp.r.x = t3.x + x;
        rp.r.y = t3.y + y;
        rp.r.w = t3.w * t3.ws;
        rp.r.h = t3.h * t3.hs;

        if (t3.flags == 1127481344) //ToDo ? 1127481344 3266576384 3258187776 (flags)
        {
            rp.r.x += rp.r.w;
            rp.r.w = -rp.r.w;
        }

        switch(t3.align)
        {
            case align_top_left: break;
            case align_bottom_left: rp.r.y -= t3.h * t3.hs; break;
            case align_top_right: rp.r.x -= t3.w * t3.ws; break;
            case align_bottom_right: rp.r.x -= t3.w * t3.ws, rp.r.y -= t3.h * t3.hs; break;
            case align_center: rp.r.x -= t3.w/2 * t3.ws, rp.r.y -=t3.h/2 * t3.hs; break;
        };

        assert(tex_idx < 0 || tex_idx == e.tex_idx);

        tex_idx = e.tex_idx;
        rects.push_back(rp);

        //printf("%d %d | %f %f\n", e.w, e.h, t3.x, t3.y);
    }

    /*
    for (auto &t4: h.type4)
    {
        nya_math::vec2 pos(t4.x + x, t4.y + y);
        nya_math::vec2 left(5.0, 0.0);
        nya_math::vec2 top(0.0, 5.0);

        nya_math::vec4 red(1.0, 0.0, 0.0, 1.0);
        std::vector<nya_math::vec2> lines;
        lines.push_back(pos + left + top);
        lines.push_back(pos - left - top);
        r.draw(lines, red);
        lines.clear();
        lines.push_back(pos - left + top);
        lines.push_back(pos + left - top);
        r.draw(lines, red);
    }
    */

    if (tex_idx >= 0)
        r.draw(rects, m_textures[tex_idx], color);
}

//------------------------------------------------------------

void tiles::debug_draw_tx(const render &r)
{
    int y = 0, idx = 0;
    std::vector<rect_pair> rects(1);
    for (auto &tx: m_uitxs)
    {
        int height = 0, x = 0;
        for (auto &e: tx.entries)
        {
            if (x + e.w > r.get_width())
            {
                y += height + 5;
                x = height = 0;
            }

            rects[0].tc.x = e.x;
            rects[0].tc.y = e.y;
            rects[0].tc.w = e.w;
            rects[0].tc.h = e.h;
            rects[0].r = rects[0].tc;
            rects[0].r.x = x;
            rects[0].r.y = y;
            if (e.h > height)
                height = e.h;
            x += e.w + 5;

            r.draw(rects, m_textures[e.tex_idx]);
        }
        y += height + 5;
        ++idx;
    }
}

//------------------------------------------------------------
}