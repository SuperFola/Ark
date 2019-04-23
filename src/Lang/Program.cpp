#include <Ark/Lang/Program.hpp>

#include <Ark/Log.hpp>
#include <Ark/Lang/Lib.hpp>

namespace Ark
{
    namespace Lang
    {
        Program::Program()
        {}

        Program::~Program()
        {}

        void Program::feed(const std::string& code)
        {
            m_parser.feed(code);
            
            if (!m_parser.check())
            {
                Ark::Log::error("[Program] Program has errors");
                exit(1);
            }
        }

        void Program::execute(const Nodes& args)
        {
            Node argslist(NodeType::List);
            for (auto& node: args)
                argslist.push_back(node);

            m_global_env["_args"] = argslist;
            registerLib(m_global_env);

            std::cout << _execute(m_parser.ast(), &m_global_env) << std::endl;
        }

        void Program::loadFunction(const std::string& name, Node::ProcType function)
        {
            m_global_env[name] = Node(function);
        }

        Node Program::_execute(Node x, Environment* env)
        {
            if (x.nodeType() == NodeType::Symbol)
            {
                std::string name = x.getStringVal();
                return env->find(name)[name];
            }
            if (x.nodeType() == NodeType::String || x.nodeType() == NodeType::Number)
                return x;
            if (x.list().empty())
                return nil;
            if (x.list()[0].nodeType() == NodeType::Keyword)
            {
                Keyword n = x.list()[0].keyword();

                if (n == Keyword::If)
                    return _execute((_execute(x.list()[1], env) == falseSym) ? x.list()[3] : x.list()[2], env);
                if (n == Keyword::Set)
                {
                    std::string name = x.list()[1].getStringVal();
                    return env->find(name)[name] = _execute(x.list()[2], env);
                }
                if (n == Keyword::Def)
                    return (*env)[x.list()[1].getStringVal()] = _execute(x.list()[2], env);
                if (n == Keyword::Fun)
                {
                    x.setNodeType(NodeType::Lambda);
                    x.addEnv(env);
                    return x;
                }
                if (n == Keyword::Begin)
                {
                    for (std::size_t i=1; i < x.list().size() - 1; ++i)
                        _execute(x.list()[i], env);
                    return _execute(x.list()[x.list().size() - 1], env);
                }
                if (n == Keyword::While)
                {
                    while (_execute(x.list()[1], env) == trueSym)
                        _execute(x.list()[2], env);
                    return nil;
                }
            }

            Node proc(_execute(x.list()[0], env));
            Nodes exps;
            for (Node::Iterator exp=x.list().begin() + 1; exp != x.list().end(); ++exp)
                exps.push_back(_execute(*exp, env));

            if (proc.nodeType() == NodeType::Lambda)
                return _execute(proc.list()[2], new Environment(proc.list()[1].list(), exps, proc.getEnv()));
            else if (proc.nodeType() == NodeType::Proc)
                return proc.call(exps);
            else
            {
                Ark::Log::error("(Program) not a function");
                exit(1);
            }
        }

        std::ostream& operator<<(std::ostream& os, const Program& P)
        {
            os << P.m_parser << std::endl;
            os << "Environment" << std::endl;
            os << P.m_global_env << std::endl;

            return os;
        }
    }
}