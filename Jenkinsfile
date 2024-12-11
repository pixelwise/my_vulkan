#!groovy
pipeline {
    environment {
        SLACK_NOTIFY_CHANNEL = 'core'
        SLACK_NOTIFY_BRANCHES = 'develop,master'
        SLACK_NOTIFY_JOB_INFO = "${env.JOB_NAME} [${env.BUILD_NUMBER}] (${env.BUILD_URL})"
    }

    options { buildDiscarder(logRotator(numToKeepStr: '10', artifactNumToKeepStr: '1')) }
    agent { label 'current' }

    stages {
        stage('prepare') {
            steps {
                script {
                    conditionalSlackSend(color: '#FFFF00', msgPrefix: 'STARTED')
                }
                sh './jenkins/prepare.sh'
                sshagent(credentials: ['game-on-ci-agent']) {
                    sh './update_submodules.sh'
                }
            }
        }
        stage('build') {
            steps {
                sh 'PATH=$PATH:~/.local/bin ./jenkins/build.sh'
            }
        }
        stage('test') {
            steps {
                sh 'PATH=$PATH:~/.local/bin ./jenkins/test.sh'
            }
        }
//        stage('package') {
//            steps {
//                sh 'PATH=$PATH:~/.local/bin ./jenkins/package.sh'
//            }
//        }
    }

    post {
        success {
            script {
                conditionalSlackSend(color: 'good', msgPrefix: 'SUCCESSFUL')
            }
         }
        failure {
            script {
                conditionalSlackSend(color: 'danger', msgPrefix: 'FAILED')
            }
            emailext(
                    body: "${currentBuild.currentResult}: Job ${env.JOB_NAME} build ${env.BUILD_NUMBER} ${env.BUILD_URL}",
                    recipientProviders: [[$class: 'DevelopersRecipientProvider'], [$class: 'RequesterRecipientProvider']],
                    subject: "Jenkins Build ${currentBuild.currentResult}: Job ${env.JOB_NAME}"
            )
        }
//         always {
//             archiveArtifacts artifacts: 'build/*.rpm', fingerprint: true
//         }
    }
}

void conditionalSlackSend(Map args) {
    if (SLACK_NOTIFY_BRANCHES.tokenize(',').contains(env.BRANCH_NAME)) {
        slackSend(
                channel: "#${SLACK_NOTIFY_CHANNEL}",
                color: args.color,
                message: "${args.msgPrefix}: ${SLACK_NOTIFY_JOB_INFO}"
        )
    }
}
