// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

namespace ospray {
  namespace sg {

    ///////////////////////////////////////////////////////////////////////////
    // Inlined definitions ////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename T>
    inline std::shared_ptr<T> Node::nodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use nodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::static_pointer_cast<T>(shared_from_this());
    }

    template <typename T>
    inline std::shared_ptr<T> Node::tryNodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use tryNodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    //! just for convenience; add a typed 'setParam' function
    template <typename T>
    inline Node &Node::createChildWithValue(const std::string &name,
                                            const std::string &type,
                                            const T &t)
    {
      if (hasChild(name)) {
        auto &c = child(name);
        c.setValue(t);
        return c;
      } else {
        auto node = createNode(name, type, t);
        add(node);
        return *node;
      }
    }

    template <typename T>
    inline void Node::setValue(T _val)
    {
      Any val(_val);
      bool modified = false;
      if (val != properties.value) {
        properties.value = val;
        modified         = true;
      }

      if (modified)
        markAsModified();
    }

    template <>
    inline void Node::setValue(Any val)
    {
      bool modified = false;
      if (val != properties.value) {
        properties.value = val;
        modified         = true;
      }

      if (modified)
        markAsModified();
    }

    template <typename T>
    inline T &Node::valueAs()
    {
      return properties.value.get<T>();
    }

    template <typename T>
    inline const T &Node::valueAs() const
    {
      return properties.value.get<T>();
    }

    template <typename T>
    inline bool Node::valueIsType() const
    {
      return properties.value.is<T>();
    }

    template <typename T>
    inline void Node::operator=(T &&v)
    {
      setValue(std::forward<T>(v));
    }

    // NOTE(jda) - Specialize valueAs() and operator=() so we don't have to
    //             convert to/from OSPObject manually, must trust the user to
    //             store/get the right type of OSPObject. This is because
    //             ospcommon::utility::Any<> cannot do implicit conversion...

#define DECLARE_VALUEAS_SPECIALIZATION(a)                \
  template <>                                            \
  inline a &Node::valueAs()                              \
  {                                                      \
    return (a &)properties.value.get<OSPObject>();       \
  }                                                      \
                                                         \
  template <>                                            \
  inline const a &Node::valueAs() const                  \
  {                                                      \
    return (const a &)properties.value.get<OSPObject>(); \
  }                                                      \
                                                         \
  template <>                                            \
  inline void Node::operator=(a &&v)                     \
  {                                                      \
    setValue((OSPObject)v);                              \
  }                                                      \
                                                         \
  template <>                                            \
  inline void Node::setValue(a val)                      \
  {                                                      \
    setValue((OSPObject)val);                            \
  }

    DECLARE_VALUEAS_SPECIALIZATION(OSPDevice)
    DECLARE_VALUEAS_SPECIALIZATION(OSPFrameBuffer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPRenderer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPCamera)
    DECLARE_VALUEAS_SPECIALIZATION(OSPWorld)
    DECLARE_VALUEAS_SPECIALIZATION(OSPData)
    DECLARE_VALUEAS_SPECIALIZATION(OSPGeometry)
    DECLARE_VALUEAS_SPECIALIZATION(OSPGeometryInstance)
    DECLARE_VALUEAS_SPECIALIZATION(OSPMaterial)
    DECLARE_VALUEAS_SPECIALIZATION(OSPLight)
    DECLARE_VALUEAS_SPECIALIZATION(OSPVolume)
    DECLARE_VALUEAS_SPECIALIZATION(OSPVolumeInstance)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTransferFunction)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTexture)
    DECLARE_VALUEAS_SPECIALIZATION(OSPPixelOp)
    DECLARE_VALUEAS_SPECIALIZATION(OSPFuture)

#undef DECLARE_VALUEAS_SPECIALIZATION

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor, TraversalContext &ctx)
    {
      static_assert(is_valid_visitor<VISITOR_T>::value,
                    "VISITOR_T must be a child class of sg::Visitor or"
                    " implement 'bool visit(Node &node, TraversalContext &ctx)'"
                    "!");

      bool traverseChildren = visitor(*this, ctx);

      ctx.level++;

      if (traverseChildren) {
        for (auto &child : properties.children)
          child.second->traverse(visitor, ctx);
      }

      ctx.level--;

      visitor.postChildren(*this, ctx);
    }

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor)
    {
      TraversalContext ctx;
      traverse(std::forward<VISITOR_T>(visitor), ctx);
    }

  }  // namespace sg
}  // namespace ospray