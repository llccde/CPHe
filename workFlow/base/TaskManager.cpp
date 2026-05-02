#include "TaskManager.h"
#include"TaskFactory.h"
#include"Intent.h"
#include"qvector.h"
#include"workFlowContext.h"
template<class T>
inline TaskNameInFactory<T>::~TaskNameInFactory()
{
}

void TaskLauncher::AddIntent(QString describe)
{
    currentIntents.push_back(
        std::unique_ptr<Intent>(
            new Intent(
                Intent::getIntentByDescribe(describe, context)
            )));
}

void TaskLauncher::launch()
{
    assert(curentOperator);
    QVector<Intent*> intents;
    for (auto& i: currentIntents)
    {
        intents.append(i.get());
    }
    curentOperator->handleIntent(intents, context);
}

void TaskLauncher::store(QString des)
{
    assert(curentOperator);
    std::unique_ptr<Intent> i(new Intent(Intent::getIntentByDescribe(des,context)));
    curentOperator->BeRememberAsIntent(i.get(),context);
}

TaskLauncher::~TaskLauncher()
{
}

TaskManager::TaskManager()
{
    context = std::unique_ptr<WorkFlowContext>(new WorkFlowContext());
}

TaskManager::~TaskManager()
{
}

void TaskManager::registTaskFactory(std::unique_ptr<TaskFactory> fac)
{
    assert(fac);
    fac->setMainContext(context.get());
    for (auto& i: fac->getOperators()){
        if (operatorNames.contains(i)) {
            assert(false);
        }
        operatorNames.insert(i,fac.get());
    }
    factorys.push_back(std::move(fac));
}

std::unique_ptr<TaskLauncher> TaskManager::prepareOperator(QString name)
{
    if (auto fac = operatorNames[name]) {
        std::unique_ptr<TaskLauncher> launcher(
            new TaskLauncher(
                context.get(),
                std::unique_ptr<BaseOperator>(fac->getOperator(name)
                )));
        return launcher;
    }
    return nullptr;
}

std::vector<TaskNameInFactory<QString>> TaskManager::getAllOperator()
{
    std::vector<TaskNameInFactory<QString>> res;
    for (auto& fac: factorys){
        TaskNameInFactory<QString>facOpts(fac->myName());
        for (auto opt:fac->getOperators())
        {
            facOpts.items.push_back(opt);
        }
        res.push_back(facOpts);
    }
    return res;
}

