#include "assimp_core.hpp"

#include <assimp/quaternion.h>
#include <cmath>

namespace AssimpBindings
{
    static double get_ticks_per_second(const aiAnimation *anim)
    {
        if (!anim || anim->mTicksPerSecond <= 0.0)
            return 25.0;
        return anim->mTicksPerSecond;
    }

    static double to_anim_time_ticks(const aiAnimation *anim, double seconds, bool loop)
    {
        if (!anim) return 0.0;

        double tps = get_ticks_per_second(anim);
        double ticks = seconds * tps;
        double duration = anim->mDuration;

        if (duration <= 0.0)
            return 0.0;

        if (loop)
        {
            double wrapped = fmod(ticks, duration);
            return wrapped < 0.0 ? wrapped + duration : wrapped;
        }

        if (ticks < 0.0) return 0.0;
        if (ticks > duration) return duration;
        return ticks;
    }

    static int find_vec_key_index(const aiVectorKey *keys, unsigned int count, double timeTicks)
    {
        if (!keys || count < 2) return 0;
        for (unsigned int i = 0; i + 1 < count; ++i)
        {
            if (timeTicks < keys[i + 1].mTime)
                return (int)i;
        }
        return (int)count - 2;
    }

    static int find_quat_key_index(const aiQuatKey *keys, unsigned int count, double timeTicks)
    {
        if (!keys || count < 2) return 0;
        for (unsigned int i = 0; i + 1 < count; ++i)
        {
            if (timeTicks < keys[i + 1].mTime)
                return (int)i;
        }
        return (int)count - 2;
    }

    static aiVector3D sample_position(const aiNodeAnim *channel, double timeTicks)
    {
        if (!channel || channel->mNumPositionKeys == 0)
            return aiVector3D(0.0f, 0.0f, 0.0f);
        if (channel->mNumPositionKeys == 1)
            return channel->mPositionKeys[0].mValue;

        int i = find_vec_key_index(channel->mPositionKeys, channel->mNumPositionKeys, timeTicks);
        const aiVectorKey &a = channel->mPositionKeys[i];
        const aiVectorKey &b = channel->mPositionKeys[i + 1];

        double dt = b.mTime - a.mTime;
        float f = 0.0f;
        if (dt > 1e-9)
            f = (float)((timeTicks - a.mTime) / dt);

        return a.mValue + (b.mValue - a.mValue) * f;
    }

    static aiVector3D sample_scaling(const aiNodeAnim *channel, double timeTicks)
    {
        if (!channel || channel->mNumScalingKeys == 0)
            return aiVector3D(1.0f, 1.0f, 1.0f);
        if (channel->mNumScalingKeys == 1)
            return channel->mScalingKeys[0].mValue;

        int i = find_vec_key_index(channel->mScalingKeys, channel->mNumScalingKeys, timeTicks);
        const aiVectorKey &a = channel->mScalingKeys[i];
        const aiVectorKey &b = channel->mScalingKeys[i + 1];

        double dt = b.mTime - a.mTime;
        float f = 0.0f;
        if (dt > 1e-9)
            f = (float)((timeTicks - a.mTime) / dt);

        return a.mValue + (b.mValue - a.mValue) * f;
    }

    static aiQuaternion sample_rotation(const aiNodeAnim *channel, double timeTicks)
    {
        if (!channel || channel->mNumRotationKeys == 0)
            return aiQuaternion();
        if (channel->mNumRotationKeys == 1)
            return channel->mRotationKeys[0].mValue;

        int i = find_quat_key_index(channel->mRotationKeys, channel->mNumRotationKeys, timeTicks);
        const aiQuatKey &a = channel->mRotationKeys[i];
        const aiQuatKey &b = channel->mRotationKeys[i + 1];

        double dt = b.mTime - a.mTime;
        float f = 0.0f;
        if (dt > 1e-9)
            f = (float)((timeTicks - a.mTime) / dt);

        aiQuaternion out;
        aiQuaternion::Interpolate(out, a.mValue, b.mValue, f);
        out.Normalize();
        return out;
    }

    static aiMatrix4x4 compose_node_anim_matrix(const aiNodeAnim *channel, double timeTicks)
    {
        aiVector3D pos = sample_position(channel, timeTicks);
        aiVector3D scl = sample_scaling(channel, timeTicks);
        aiQuaternion rot = sample_rotation(channel, timeTicks);

        aiMatrix4x4 mT;
        aiMatrix4x4::Translation(pos, mT);

        aiMatrix4x4 mR(rot.GetMatrix());

        aiMatrix4x4 mS;
        aiMatrix4x4::Scaling(scl, mS);

        return mT * mR * mS;
    }

    static const aiNodeAnim *find_channel_by_name(const aiAnimation *anim, const char *nodeName)
    {
        if (!anim || !nodeName) return nullptr;
        for (unsigned int i = 0; i < anim->mNumChannels; ++i)
        {
            const aiNodeAnim *ch = anim->mChannels[i];
            if (ch && strcmp(ch->mNodeName.C_Str(), nodeName) == 0)
                return ch;
        }
        return nullptr;
    }

    static void *anim_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        Error("AssimpAnimation cannot be constructed directly; use AssimpScene.getAnimation()");
        return nullptr;
    }

    static void anim_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        AnimationHandle *h = (AnimationHandle *)instance;
        delete h;
    }

    static void *node_anim_ctor_error(Interpreter *vm, int argCount, Value *args)
    {
        (void)vm; (void)argCount; (void)args;
        Error("AssimpNodeAnim cannot be constructed directly; use AssimpAnimation.getChannel()");
        return nullptr;
    }

    static void node_anim_dtor(Interpreter *vm, void *instance)
    {
        (void)vm;
        NodeAnimHandle *h = (NodeAnimHandle *)instance;
        delete h;
    }

    static int anim_get_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getName()");
        if (!h) return push_nil1(vm);
        vm->pushString(h->anim->mName.C_Str());
        return 1;
    }

    static int anim_get_duration_ticks(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getDurationTicks()");
        if (!h) return push_nil1(vm);
        vm->pushDouble(h->anim->mDuration);
        return 1;
    }

    static int anim_get_ticks_per_second(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getTicksPerSecond()");
        if (!h) return push_nil1(vm);
        vm->pushDouble(get_ticks_per_second(h->anim));
        return 1;
    }

    static int anim_get_duration_seconds(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getDurationSeconds()");
        if (!h) return push_nil1(vm);

        double tps = get_ticks_per_second(h->anim);
        double secs = (tps > 0.0) ? (h->anim->mDuration / tps) : 0.0;
        vm->pushDouble(secs);
        return 1;
    }

    static int anim_get_channel_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getChannelCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->anim->mNumChannels);
        return 1;
    }

    static int anim_get_channel(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpAnimation.getChannel(index)"); return push_nil1(vm); }
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.getChannel()");
        if (!h) return push_nil1(vm);

        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpAnimation.getChannel()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->anim->mNumChannels)
        {
            Error("AssimpAnimation.getChannel(): index %d out of range", idx);
            return push_nil1(vm);
        }

        return push_node_anim(vm, h->owner, h->anim, h->anim->mChannels[idx]) ? 1 : push_nil1(vm);
    }

    static int anim_find_channel(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpAnimation.findChannel(nodeName)"); return push_nil1(vm); }
        AnimationHandle *h = require_animation(instance, "AssimpAnimation.findChannel()");
        if (!h) return push_nil1(vm);

        const char *nodeName = nullptr;
        if (!read_string_arg(args[0], &nodeName, "AssimpAnimation.findChannel()", 1)) return push_nil1(vm);
        const aiNodeAnim *ch = find_channel_by_name(h->anim, nodeName);
        if (!ch) return push_nil1(vm);

        return push_node_anim(vm, h->owner, h->anim, ch) ? 1 : push_nil1(vm);
    }

    static int anim_sample_node_transform(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 2 || argCount > 3)
        {
            Error("AssimpAnimation.sampleNodeTransform(nodeName, timeSeconds [, loop])");
            return push_nil1(vm);
        }

        AnimationHandle *h = require_animation(instance, "AssimpAnimation.sampleNodeTransform()");
        if (!h) return push_nil1(vm);

        const char *nodeName = nullptr;
        double timeSec = 0.0;
        bool loop = true;

        if (!read_string_arg(args[0], &nodeName, "AssimpAnimation.sampleNodeTransform()", 1) ||
            !read_number_arg(args[1], &timeSec, "AssimpAnimation.sampleNodeTransform()", 2))
        {
            return push_nil1(vm);
        }
        if (argCount > 2 && !read_boolish_arg(args[2], &loop, "AssimpAnimation.sampleNodeTransform()", 3))
            return push_nil1(vm);

        const aiNodeAnim *ch = find_channel_by_name(h->anim, nodeName);
        if (!ch) return push_nil1(vm);

        double ticks = to_anim_time_ticks(h->anim, timeSec, loop);
        aiMatrix4x4 m = compose_node_anim_matrix(ch, ticks);
        return push_matrix(vm, m) ? 1 : push_nil1(vm);
    }

    static int node_anim_get_node_name(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getNodeName()");
        if (!h) return push_nil1(vm);
        vm->pushString(h->channel->mNodeName.C_Str());
        return 1;
    }

    static int node_anim_get_position_key_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getPositionKeyCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->channel->mNumPositionKeys);
        return 1;
    }

    static int node_anim_get_rotation_key_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getRotationKeyCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->channel->mNumRotationKeys);
        return 1;
    }

    static int node_anim_get_scaling_key_count(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getScalingKeyCount()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->channel->mNumScalingKeys);
        return 1;
    }

    static int node_anim_get_position_key(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpNodeAnim.getPositionKey(index)"); return push_nil1(vm); }
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getPositionKey()");
        if (!h) return push_nil1(vm);

        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpNodeAnim.getPositionKey()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->channel->mNumPositionKeys) return push_nil1(vm);

        const aiVectorKey &k = h->channel->mPositionKeys[idx];
        vm->pushDouble(k.mTime);
        vm->pushDouble(k.mValue.x);
        vm->pushDouble(k.mValue.y);
        vm->pushDouble(k.mValue.z);
        return 4;
    }

    static int node_anim_get_rotation_key(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpNodeAnim.getRotationKey(index)"); return push_nil1(vm); }
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getRotationKey()");
        if (!h) return push_nil1(vm);

        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpNodeAnim.getRotationKey()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->channel->mNumRotationKeys) return push_nil1(vm);

        const aiQuatKey &k = h->channel->mRotationKeys[idx];
        vm->pushDouble(k.mTime);
        vm->pushDouble(k.mValue.x);
        vm->pushDouble(k.mValue.y);
        vm->pushDouble(k.mValue.z);
        vm->pushDouble(k.mValue.w);
        return 5;
    }

    static int node_anim_get_scaling_key(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount != 1) { Error("AssimpNodeAnim.getScalingKey(index)"); return push_nil1(vm); }
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getScalingKey()");
        if (!h) return push_nil1(vm);

        double idxd = 0.0;
        if (!read_number_arg(args[0], &idxd, "AssimpNodeAnim.getScalingKey()", 1)) return push_nil1(vm);
        int idx = (int)idxd;
        if (idx < 0 || idx >= (int)h->channel->mNumScalingKeys) return push_nil1(vm);

        const aiVectorKey &k = h->channel->mScalingKeys[idx];
        vm->pushDouble(k.mTime);
        vm->pushDouble(k.mValue.x);
        vm->pushDouble(k.mValue.y);
        vm->pushDouble(k.mValue.z);
        return 4;
    }

    static int node_anim_get_pre_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getPreState()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->channel->mPreState);
        return 1;
    }

    static int node_anim_get_post_state(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        (void)argCount; (void)args;
        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.getPostState()");
        if (!h) return push_nil1(vm);
        vm->pushInt((int)h->channel->mPostState);
        return 1;
    }

    static int node_anim_sample_transform(Interpreter *vm, void *instance, int argCount, Value *args)
    {
        if (argCount < 1 || argCount > 2)
        {
            Error("AssimpNodeAnim.sampleTransform(timeSeconds [, loop])");
            return push_nil1(vm);
        }

        NodeAnimHandle *h = require_node_anim(instance, "AssimpNodeAnim.sampleTransform()");
        if (!h) return push_nil1(vm);

        double timeSec = 0.0;
        bool loop = true;
        if (!read_number_arg(args[0], &timeSec, "AssimpNodeAnim.sampleTransform()", 1)) return push_nil1(vm);
        if (argCount > 1 && !read_boolish_arg(args[1], &loop, "AssimpNodeAnim.sampleTransform()", 2)) return push_nil1(vm);

        double ticks = to_anim_time_ticks(h->anim, timeSec, loop);
        aiMatrix4x4 m = compose_node_anim_matrix(h->channel, ticks);
        return push_matrix(vm, m) ? 1 : push_nil1(vm);
    }

    void register_animation(Interpreter &vm)
    {
        g_animationClass = vm.registerNativeClass("AssimpAnimation", anim_ctor_error, anim_dtor, 0, false);
        vm.addNativeMethod(g_animationClass, "getName", anim_get_name);
        vm.addNativeMethod(g_animationClass, "getDurationTicks", anim_get_duration_ticks);
        vm.addNativeMethod(g_animationClass, "getTicksPerSecond", anim_get_ticks_per_second);
        vm.addNativeMethod(g_animationClass, "getDurationSeconds", anim_get_duration_seconds);
        vm.addNativeMethod(g_animationClass, "getChannelCount", anim_get_channel_count);
        vm.addNativeMethod(g_animationClass, "getChannel", anim_get_channel);
        vm.addNativeMethod(g_animationClass, "findChannel", anim_find_channel);
        vm.addNativeMethod(g_animationClass, "sampleNodeTransform", anim_sample_node_transform);

        g_nodeAnimClass = vm.registerNativeClass("AssimpNodeAnim", node_anim_ctor_error, node_anim_dtor, 0, false);
        vm.addNativeMethod(g_nodeAnimClass, "getNodeName", node_anim_get_node_name);
        vm.addNativeMethod(g_nodeAnimClass, "getPositionKeyCount", node_anim_get_position_key_count);
        vm.addNativeMethod(g_nodeAnimClass, "getRotationKeyCount", node_anim_get_rotation_key_count);
        vm.addNativeMethod(g_nodeAnimClass, "getScalingKeyCount", node_anim_get_scaling_key_count);
        vm.addNativeMethod(g_nodeAnimClass, "getPositionKey", node_anim_get_position_key);
        vm.addNativeMethod(g_nodeAnimClass, "getRotationKey", node_anim_get_rotation_key);
        vm.addNativeMethod(g_nodeAnimClass, "getScalingKey", node_anim_get_scaling_key);
        vm.addNativeMethod(g_nodeAnimClass, "getPreState", node_anim_get_pre_state);
        vm.addNativeMethod(g_nodeAnimClass, "getPostState", node_anim_get_post_state);
        vm.addNativeMethod(g_nodeAnimClass, "sampleTransform", node_anim_sample_transform);
    }

} // namespace AssimpBindings
