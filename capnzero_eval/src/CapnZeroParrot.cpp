#include "capnzero-eval-msgs/EvalMessageCapnZero.capnp.h"
#include "capnzero_eval/EvalMessageRos.h"

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnzero/Common.h>
#include <capnzero/Publisher.h>
#include <capnzero/Subscriber.h>
#include <kj/array.h>
#include <signal.h>

#include <ros/ros.h>

#include <vector>

static bool interrupted = false;
static void s_signal_handler(int signal_value)
{
    interrupted = true;
}
static void s_catch_signals(void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

void evalRos(int argc, char** argv, std::string topic);
void callbackRos(const capnzero_eval::EvalMessageRos::ConstPtr& msg);
ros::Publisher pubRos;

void callbackCapnZero(::capnp::FlatArrayMessageReader& reader);
void evalCapnZero(std::string topic);
capnzero::Publisher* pubCapnZero;

int main(int argc, char** argv)
{
    s_catch_signals();

    if (argc <= 1) {
        std::cerr << "Synopsis: rosrun capnzero_eval Parrot [ros|capnzero] <Topic>" << std::endl;
        return -1;
    }

    for (size_t i = 0; i < argc; i++) {
        std::cout << "Param " << i << ": '" << argv[i] << "'" << std::endl;
    }

    if (std::string("ros").compare(argv[1]) == 0) {
        evalRos(argc, argv, argv[2]);
    } else {
        evalCapnZero(argv[2]);
    }
}

void evalRos(int argc, char** argv, std::string topic)
{
    ros::init(argc, argv, "Parrot");
    ros::NodeHandle n;
    pubRos = n.advertise<capnzero_eval::EvalMessageRos>(topic+"back", 1000);
    ros::Subscriber sub = n.subscribe(topic, 1, callbackRos);
    ros::Rate loop_rate(100.0);
    while (ros::ok()) {
        ros::AsyncSpinner spinner(4);
        spinner.start();
        loop_rate.sleep();
    }
}

void callbackRos(const capnzero_eval::EvalMessageRos::ConstPtr& msg)
{
    pubRos.publish(msg);
}

void evalCapnZero(std::string topic)
{
    void* ctx = zmq_ctx_new();
    //    capnzero::Subscriber* sub = new capnzero::Subscriber(ctx, capnzero::Protocol::UDP);
    //    sub->addAddress("224.0.0.2:5500");
    //    capnzero::Subscriber* sub = new capnzero::Subscriber(ctx, capnzero::Protocol::IPC);
    //    sub->addAddress("@capnzeroSend.ipc");
    capnzero::Subscriber* sub = new capnzero::Subscriber(ctx, capnzero::Protocol::TCP);
    sub->addAddress("127.0.0.1:5500");

    sub->setTopic(topic);
    sub->subscribe(&callbackCapnZero);

    //    pub = new capnzero::Publisher(ctx, capnzero::Protocol::UDP);
    //    pub->addAddress("224.0.0.2:5554");
    //    pub = new capnzero::Publisher(ctx, capnzero::Protocol::IPC);
    //    pub->addAddress("@capnzeroReceive.ipc");
    pubCapnZero = new capnzero::Publisher(ctx, capnzero::Protocol::TCP);
    pubCapnZero->addAddress("127.0.0.1:5554");

    pubCapnZero->setDefaultTopic(topic);

    while (!interrupted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete sub;
    delete pubCapnZero;
    zmq_ctx_term(ctx);
}

void callbackCapnZero(::capnp::FlatArrayMessageReader& reader)
{
    ::capnp::MallocMessageBuilder msgBuilder;
    //    capnzero_eval::EvalMessage::Builder msg = msgBuilder.initRoot<capnzero_eval::EvalMessage>();
    //    msg.setPayload(reader.getRoot<capnzero_eval::EvalMessage>().getPayload());
    //    msg.setId(reader.getRoot<capnzero_eval::EvalMessage>().getId());
    msgBuilder.setRoot<capnzero_eval::EvalMessageCapnZero::Reader>(reader.getRoot<capnzero_eval::EvalMessageCapnZero>());
    pubCapnZero->send(msgBuilder);
}